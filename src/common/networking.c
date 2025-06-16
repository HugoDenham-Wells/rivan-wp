#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "networking.h"

#define RETRY_DELAY_SEC 1

void send_packet(int sock, const uint8_t *data, size_t len)
{
    send(sock, data, len, 0);
    printf("Sent: ");
    for (size_t i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
    usleep(50000); // brief pause between packets
}

int connect_with_retry(const char *host, int port, const char *name)
{
    int sock;
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(port)};
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    while (1)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("socket");
            printf("Connection failed, retrying in %d seconds...\n", RETRY_DELAY_SEC);
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
        {
            printf("Connected to %s:%d\n", host, port);
            // Send identification string - CMD expects it
            // TODO - this might be hacky, we should consider a true protocol.
            // Also a bug is here - we are checking this string in an unsafe way.
            if (name != NULL)
            {
                send(sock, name, strlen(name), 0);
            }
            return sock;
        }

        perror("connect");
        close(sock);
        sleep(RETRY_DELAY_SEC);
    }
}
