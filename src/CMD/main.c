#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define RT_EXE_PORT 17725
#define RT_EXE_ADDR "127.0.0.1" // Change if RT_EXE is remote
#define RECONNECT_DELAY_SEC 2

static volatile int keep_running = 1;

void handle_sigint(int sig) {
    (void)sig;
    printf("Received SIGINT, shutting down...\n");
    // Set keep_running to 0 to exit the main loop
    keep_running = 0;
}

int connect_to_rtexe() {
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RT_EXE_PORT);
    if (inet_pton(AF_INET, RT_EXE_ADDR, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

int main(void) {
    signal(SIGINT, handle_sigint);

    int sock = -1;

    while (keep_running) {
        if (sock < 0) {
            printf("Connecting to RT_EXE at %s:%d...\n", RT_EXE_ADDR, RT_EXE_PORT);
            sock = connect_to_rtexe();
            if (sock < 0) {
                printf("Connection failed, retrying in %d seconds...\n", RECONNECT_DELAY_SEC);
                sleep(RECONNECT_DELAY_SEC);
                continue;
            }
            printf("Connected to RT_EXE.\n");
        }

        // Placeholder for future packet handling (requirements 2-4)
        char buf[256];
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n == 0) {
            printf("RT_EXE closed connection. Reconnecting...\n");
            close(sock);
            sock = -1;
            continue;
        } else if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            close(sock);
            sock = -1;
            continue;
        }
        // For now, just print received data
        printf("Received %zd bytes from RT_EXE\n", n);
    }

    if (sock >= 0) close(sock);
    printf("CMD exiting.\n");
    return 0;
}