#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "networking.h"

static volatile int keep_running = 1;

void handle_sigint(int sig)
{
    (void)sig;
    printf("Received SIGINT, shutting down...\n");
    // Set keep_running to 0 to exit the main loop
    keep_running = 0;
}

void save_data_to_file(const uint8_t *data, size_t len)
{
    FILE *file = fopen("xcp_data.bin", "ab");
    if (!file)
    {
        perror("Failed to open xcp_data.bin");
        return;
    }
    fwrite(data, 1, len, file);
    fclose(file);
}

void decode_xcp_packet(const uint8_t *data, size_t len)
{
    // This function should decode the XCP packet according to the XCP protocol.
    // For now, we will just print the raw data in hex format.
    printf("XCP Packet: ");
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");

    save_data_to_file(data, len);
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    int cmd_fd = -1;

    while (keep_running)
    {

        if (cmd_fd < 0)
        {
            cmd_fd = connect_with_retry(CMD_ADDR, CMD_PORT, "XCP_DAQ");
            continue;
        }
        printf("Connected to CMD at %s:%d\n", CMD_ADDR, CMD_PORT);

        // Wait for data from CMD
        uint8_t buffer[256];
        size_t n = recv(cmd_fd, buffer, sizeof(buffer), 0);

        if (n < 0)
        {
            perror("recv");
            close(cmd_fd);
            cmd_fd = -1;
            continue; // Retry connection
        }
        else if (n == 0)
        {
            printf("CMD connection closed by peer.\n");
            close(cmd_fd);
            cmd_fd = -1;
            continue; // Retry connection
        }

        // Process the received data
        // decode_xcp_packet(buffer, n);
        // printf("Received %zd bytes from CMD\n", n);
    }

    return 0;
}