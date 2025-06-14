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

#include "state.h"

#define RT_EXE_PORT 17725
#define CLIENT_PORT 17726
#define RT_EXE_ADDR "127.0.0.1" // Change if RT_EXE is remote
#define RECONNECT_DELAY_SEC 2

typedef struct
{
    int rt_exe_fd;
    int xcp_cmd_fd;
    int xcp_daq_fd;
    int client_server_fd;
} SocketHandles;

static volatile int keep_running = 1;
XcpSessionState session_state;

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

int connect_to_rtexe()
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
    addr.sin_port = htons(RT_EXE_PORT);
    if (inet_pton(AF_INET, RT_EXE_ADDR, &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
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
    printf("Accepted new client connection.\n");

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
    printf("Received %zd bytes from RT_EXE\n", n);
    return 0;
}

static void main_loop(SocketHandles *sockets)
{
    while (keep_running)
    {
        printf("CMD loop iteration started.\n");

        if (sockets->rt_exe_fd < 0)
        {
            printf("Connecting to RT_EXE at %s:%d...\n", RT_EXE_ADDR, RT_EXE_PORT);
            sockets->rt_exe_fd = connect_to_rtexe();
            if (sockets->rt_exe_fd < 0)
            {
                printf("Connection failed, retrying in %d seconds...\n", RECONNECT_DELAY_SEC);
                sleep(RECONNECT_DELAY_SEC);
                continue;
            }
            printf("Connected to RT_EXE.\n");

            // If we have saved packets, send them to RT_EXE. Should reestablish the DAQ state.
            for (int i = 0; i < session_state.packet_count; ++i)
            {
                send(sockets->rt_exe_fd,
                     session_state.packets[i],
                     session_state.packet_lengths[i],
                     0);
                printf("Sent saved packet %ld bytes to RT_EXE.\n", session_state.packet_lengths[i]);
            }
        }

        fd_set readfds;
        FD_ZERO(&readfds);

        // Add all valid sockets to the listening set
        if (sockets->rt_exe_fd >= 0)
        {
            FD_SET(sockets->rt_exe_fd, &readfds);
        }
        if (sockets->xcp_cmd_fd >= 0)
        {
            FD_SET(sockets->xcp_cmd_fd, &readfds);
        }
        if (sockets->xcp_daq_fd >= 0)
        {
            FD_SET(sockets->xcp_daq_fd, &readfds);
        }
        if (sockets->client_server_fd >= 0)
        {
            FD_SET(sockets->client_server_fd, &readfds);
        }

        // Determine the maximum file descriptor for select TODO this seems yuck, but it works. Is there a better way?
        int maxfd = -1;
        if (sockets->rt_exe_fd > maxfd)
            maxfd = sockets->rt_exe_fd;
        if (sockets->client_server_fd > maxfd)
            maxfd = sockets->client_server_fd;
        if (sockets->xcp_cmd_fd > maxfd)
            maxfd = sockets->xcp_cmd_fd;
        if (sockets->xcp_daq_fd > maxfd)
            maxfd = sockets->xcp_daq_fd;
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }
        else if (ret == 0)
        {
            continue;
        }

        if (FD_ISSET(sockets->client_server_fd, &readfds))
        {
            handle_new_client(sockets);
        }

        if (FD_ISSET(sockets->rt_exe_fd, &readfds))
        {
            if (handle_rt_exe(sockets) < 0)
                continue;
        }
        if (FD_ISSET(sockets->xcp_cmd_fd, &readfds))
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
                // TODO: There is a bug here, if XCP_CMD floods us with packets,
                // we will have an ever-growing list of XCP packets in session_state.
                // Unclear what the right solution is - throw away old packets? Add a
                // reset / command header to each XCP packet?
                add_xcp_packet(&session_state, buffer, len); // Save received packet

                if (send(sockets->rt_exe_fd, buffer, len, 0) != len) // Forward to RT_EXE
                {
                    // TODO there is a failure mode here when a setup is interrupted mid-send,
                    // we could put the RT_EXE in an unknown state.
                    perror("send to RT_EXE failed");
                }
                else
                {
                    printf("Forwarded %d bytes to RT_EXE:", len);
                }
            }
        }
    }
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    if (load_xcp_state(&session_state) < 0)
    {
        fprintf(stderr, "Failed to load xcp session state, starting fresh.\n");
        memset(&session_state, 0, sizeof(session_state));
    }
    else
    {
        printf("Loaded session state successfully.\n");
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