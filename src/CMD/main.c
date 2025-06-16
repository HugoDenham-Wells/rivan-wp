#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <poll.h>

#include "state.h"
#include "networking.h"
#include "xcp_utils.h"

#define CLIENT_PORT CMD_PORT
#define RECONNECT_DELAY_SEC 2

// Files that store the cached outgoing XCP command and incoming DAQ packets.
#define XCP_CMD_CACHE_FILE "xcp_cmd_cached.bin"
#define XCP_DAQ_CACHE_FILE "xcp_daq_cached.bin"

typedef struct
{
    int rt_exe_fd;
    int xcp_cmd_fd;
    int xcp_daq_fd;
    int client_server_fd;
} SocketHandles;

static volatile int keep_running = 1;
XcpSessionState xcp_cmd_state;
XcpSessionState xcp_daq_state;

void handle_sigint(int sig)
{
    (void)sig;
    printf("Received SIGINT, shutting down...\n");
    // Set keep_running to 0 to exit the main loop
    keep_running = 0;
}

int setup_tcp_server(int port)
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    addr.sin_port = htons(port);

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return -1;
    }

    if (listen(sock, 5) < 0)
    {
        perror("listen");
        close(sock);
        return -1;
    }

    return sock;
}

static void cleanup_sockets(SocketHandles *sockets)
{
    if (sockets->rt_exe_fd >= 0)
        close(sockets->rt_exe_fd);
    if (sockets->xcp_cmd_fd >= 0)
        close(sockets->xcp_cmd_fd);
    if (sockets->xcp_daq_fd >= 0)
        close(sockets->xcp_daq_fd);
    if (sockets->client_server_fd >= 0)
        close(sockets->client_server_fd);
}

static void handle_new_client(SocketHandles *sockets)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(sockets->client_server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept");
        return;
    }
    printf("Accepted new client connection (fd: %d).\n", client_fd);

    // Wait for an initial message identifying role
    // TODO - make this more robus - is there a better way to make
    // sure we are talking to the right client?
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(client_fd, &readfds);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

    int ret = select(client_fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        if (ret == 0)
            printf("Client did not send role identifier within timeout. Closing connection.\n");
        else
            perror("select (client role)");
        close(client_fd);
        return;
    }

    char buf[32] = {0};
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
    {
        printf("Failed to receive role identifier from client.\n");
        close(client_fd);
        return;
    }

    buf[n] = '\0';
    printf("Received role identifier: %s\n", buf);

    if (strncmp(buf, "XCP_CMD", 7) == 0)
    {
        // Close previous connection if it exists, and assign to xcp_cmd_fd
        if (sockets->xcp_cmd_fd >= 0)
            close(sockets->xcp_cmd_fd);
        sockets->xcp_cmd_fd = client_fd;
        printf("Client assigned as XCP_CMD.\n");
        return;
    }
    else if (strncmp(buf, "XCP_DAQ", 7) == 0)
    {
        // Close previous connection if it exists, and assign to xcp_daq_fd
        if (sockets->xcp_daq_fd >= 0)
            close(sockets->xcp_daq_fd);
        sockets->xcp_daq_fd = client_fd;
        printf("Client assigned as XCP_DAQ.\n");

        // If we have cached DAQ packets, send them to the new XCP_DAQ client
        for (int i = 0; i < xcp_daq_state.packet_count; ++i)
        {
            if (send(sockets->xcp_daq_fd, xcp_daq_state.packets[i], xcp_daq_state.packet_lengths[i], 0) != xcp_daq_state.packet_lengths[i])
            {
                perror("send to XCP_DAQ failed");
                close(sockets->xcp_daq_fd);
                sockets->xcp_daq_fd = -1;
                return;
            }
            printf("Sent cached DAQ packet %ld bytes to XCP_DAQ.\n", xcp_daq_state.packet_lengths[i]);
        }
        // Reset the cached DAQ state after sending
        reset_xcp_state(&xcp_daq_state, XCP_DAQ_CACHE_FILE);

        return;
    }
    else
    {
        printf("Unknown role identifier: %s. Closing connection.\n", buf);
        close(client_fd);
        return;
    }
}

static int handle_rt_exe(SocketHandles *sockets)
{
    char buf[256];
    ssize_t n = recv(sockets->rt_exe_fd, buf, sizeof(buf), 0);
    if (n == 0)
    {
        printf("RT_EXE closed connection. Reconnecting...\n");
        close(sockets->rt_exe_fd);
        sockets->rt_exe_fd = -1;
        return -1;
    }
    else if (n < 0)
    {
        if (errno == EINTR)
            return 0;
        perror("recv");
        close(sockets->rt_exe_fd);
        sockets->rt_exe_fd = -1;
        return -1;
    }

    if (is_daq_packet((uint8_t *)buf, n))
    {
        if (sockets->xcp_daq_fd < 0)
        {
            printf("XCP_DAQ socket is not connected, caching DAQ packet.\n");

            add_xcp_packet(&xcp_daq_state, (uint8_t *)buf, n, XCP_DAQ_CACHE_FILE);
            return -1;
        }

        if (send(sockets->xcp_daq_fd, buf, n, 0) != n)
        {
            perror("send to XCP_DAQ failed");
            close(sockets->xcp_daq_fd);
            sockets->xcp_daq_fd = -1;
            return -1;
        }
        printf("Forwarded %zd bytes to XCP_DAQ.\n", n);
    }
    else
    {
        // Must be a CMD packet - shoot it off to XCP_CMD
        // TODO - are all non-DAQ packets CMD packets?
        if (sockets->xcp_cmd_fd < 0)
        {
            // TODO - what should we do for orphaned / unexpected / unsolicited packets?
            printf("XCP_CMD socket is not connected, discarding packet.\n");

            return -1;
        }
        else
        {
            if (send(sockets->xcp_cmd_fd, buf, n, 0) != n)
            {
                perror("send to XCP_CMD failed");
                close(sockets->xcp_cmd_fd);
                sockets->xcp_cmd_fd = -1;
                return -1;
            }
            printf("Forwarded %zd bytes to XCP_CMD.\n", n);
        }
    }
    return 0;
}

static void handle_xcp_cmd(SocketHandles *sockets)
{
    // Handle incoming XCP_CMD messages
    uint8_t buffer[256];
    int len = recv(sockets->xcp_cmd_fd, buffer, sizeof(buffer), 0);

    if (len == 0)
    {
        printf("XCP_CMD closed connection.\n");
        close(sockets->xcp_cmd_fd);
        sockets->xcp_cmd_fd = -1;
    }
    else if (len < 0)
    {
        perror("recv from XCP_CMD");
        close(sockets->xcp_cmd_fd);
        sockets->xcp_cmd_fd = -1;
    }
    else
    {
        // TODO: There is a bug here, if XCP_CMD runs a few times / repeatedly we
        // will hit cached packet limit and no longer be able to change DAQ setup state
        // Unclear what the right solution is - throw away old packets? Add a
        // reset / command header to each XCP_CMD packet?
        add_xcp_packet(&xcp_cmd_state, buffer, len, XCP_CMD_CACHE_FILE); // Save received packet

        if (send(sockets->rt_exe_fd, buffer, len, 0) != len) // Forward to RT_EXE
        {
            // TODO there is a failure mode here when a setup is interrupted mid-send,
            // we could put the RT_EXE in an unknown state.
            perror("send to RT_EXE failed");
        }
        else
        {
            printf("Forwarded %d bytes to RT_EXE\n", len);
        }
    }
}

static void main_loop(SocketHandles *sockets)
{
    while (keep_running)
    {
        // Connect to RT_EXE if needed - blocks on this.
        if (sockets->rt_exe_fd < 0)
        {
            printf("Connecting to RT_EXE at %s:%d...\n", RT_EXE_ADDR, RT_EXE_PORT);
            sockets->rt_exe_fd = connect_with_retry(RT_EXE_ADDR, RT_EXE_PORT, NULL);
            printf("Connected to RT_EXE.\n");
            for (int i = 0; i < xcp_cmd_state.packet_count; ++i)
            {
                send(sockets->rt_exe_fd,
                     xcp_cmd_state.packets[i],
                     xcp_cmd_state.packet_lengths[i],
                     0);
                printf("Sent saved packet %ld bytes to RT_EXE.\n", xcp_cmd_state.packet_lengths[i]);
            }
        }

        // Set up FD polling for our sockets
        struct pollfd pfds[4];
        int nfds = 0;

        // Add RT_EXE
        if (sockets->rt_exe_fd >= 0)
        {
            pfds[nfds].fd = sockets->rt_exe_fd;
            pfds[nfds].events = POLLIN;
            nfds++;
        }

        // Add XCP_CMD
        if (sockets->xcp_cmd_fd >= 0)
        {
            pfds[nfds].fd = sockets->xcp_cmd_fd;
            pfds[nfds].events = POLLIN;
            nfds++;
        }

        // Add XCP_DAQ
        if (sockets->xcp_daq_fd >= 0)
        {
            pfds[nfds].fd = sockets->xcp_daq_fd;
            pfds[nfds].events = POLLIN;
            nfds++;
        }

        // Add server socket (for new client connections)
        if (sockets->client_server_fd >= 0)
        {
            pfds[nfds].fd = sockets->client_server_fd;
            pfds[nfds].events = POLLIN;
            nfds++;
        }

        int ret = poll(pfds, nfds, 1000); // 1000 ms = 1 sec timeout

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }
        else if (ret == 0)
        {
            printf("No events within timeout, continuing...\n");
            // Timeout
            continue;
        }

        // Handle events in order of pfds
        int pfd_index = 0;

        // RT_EXE
        if (sockets->rt_exe_fd >= 0 && pfds[pfd_index].revents & POLLIN)
        {
            if (handle_rt_exe(sockets) < 0)
                continue; // If RT_EXE connection was closed, skip further
                          // processing and go back and wait for RT_EXE connection.
        }
        if (sockets->rt_exe_fd >= 0)
            pfd_index++;

        // XCP_CMD
        if (sockets->xcp_cmd_fd >= 0 && pfds[pfd_index].revents & POLLIN)
        {
            handle_xcp_cmd(sockets);
        }
        if (sockets->xcp_cmd_fd >= 0)
            pfd_index++;

        // XCP_DAQ (if you want to handle incoming, currently unused)
        if (sockets->xcp_daq_fd >= 0 && pfds[pfd_index].revents & POLLIN)
        {
            // TODO: Add handler if XCP_DAQ ever sends data
        }
        if (sockets->xcp_daq_fd >= 0)
            pfd_index++;

        // New client connection
        if (sockets->client_server_fd >= 0 && pfds[pfd_index].revents & POLLIN)
        {
            handle_new_client(sockets);
        }
    }
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    if (load_xcp_state(&xcp_cmd_state, XCP_CMD_CACHE_FILE) < 0)
    {
        fprintf(stderr, "Failed to load XCP_CMD session state, starting fresh.\n");
        memset(&xcp_cmd_state, 0, sizeof(xcp_cmd_state));
    }
    else
    {
        printf("Loaded XCP_CMD state successfully.\n");
    }

    if (load_xcp_state(&xcp_daq_state, XCP_DAQ_CACHE_FILE) < 0)
    {
        fprintf(stderr, "No cached XCP_DAQ packets, starting fresh.\n");
        memset(&xcp_daq_state, 0, sizeof(xcp_daq_state));
    }
    else
    {
        printf("Loaded XCP_DAQ state successfully.\n");
    }

    SocketHandles sockets = {-1, -1, -1, -1};

    sockets.client_server_fd = setup_tcp_server(CLIENT_PORT);
    if (sockets.client_server_fd < 0)
    {
        fprintf(stderr, "Failed to set up TCP server on port %d\n", CLIENT_PORT);
        return 1;
    }
    printf("TCP server listening on port %d\n", CLIENT_PORT);

    main_loop(&sockets); // Main event loop, runs until SIGINT.

    cleanup_sockets(&sockets);
    printf("CMD exiting.\n");
    return 0;
}