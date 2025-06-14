#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define HOST "127.0.0.1"
#define PORT 17726
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

int connect_with_retry(const char *host, int port)
{
    int sock;
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(port)};
    inet_pton(AF_INET, host, &addr.sin_addr);

    while (1)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("socket");
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
        {
            return sock;
        }

        perror("connect");
        close(sock);
        sleep(RETRY_DELAY_SEC);
    }
}

int main()
{
    int sock = connect_with_retry(HOST, PORT);
    printf("Connected to CMD at %s:%d\n", HOST, PORT);

    // Optional identification if CMD expects it
    send(sock, "XCP_CMD", 7, 0);

    // DAQ setup for SIMPLE_RT_Y.Q_1 and Q_2 (8-byte floats at known addresses)

    // Set DAQ pointer for entry 0 (Q_1)
    uint8_t set_daq_ptr_q1[] = {0xB2, 0x00, 0x00, 0x00}; // list 0, odt 0, entry 0
    send_packet(sock, set_daq_ptr_q1, sizeof(set_daq_ptr_q1));

    // Write Q_1 (ECU_ADDRESS 0x000305B0, 8 bytes)
    uint8_t write_daq_q1[] = {0xC1, 0x08, 0xB0, 0x05, 0x03, 0x00}; // size, address (LE)
    send_packet(sock, write_daq_q1, sizeof(write_daq_q1));

    // Set DAQ pointer for entry 1 (Q_2)
    uint8_t set_daq_ptr_q2[] = {0xB2, 0x00, 0x00, 0x01}; // list 0, odt 0, entry 1
    send_packet(sock, set_daq_ptr_q2, sizeof(set_daq_ptr_q2));

    // Write Q_2 (ECU_ADDRESS 0x000305B8, 8 bytes)
    uint8_t write_daq_q2[] = {0xC1, 0x08, 0xB8, 0x05, 0x03, 0x00};
    send_packet(sock, write_daq_q2, sizeof(write_daq_q2));

    // Set DAQ list mode (1 ODT, 1 event channel, continuous)
    uint8_t set_list_mode[] = {0xC3, 0x00, 0x01, 0x00, 0x01}; // list 0, mode=1 (continuous)
    send_packet(sock, set_list_mode, sizeof(set_list_mode));

    // Start DAQ (START_STOP_SYNCH)
    uint8_t start_daq[] = {0xC0, 0x01}; // mode=1 = start all
    send_packet(sock, start_daq, sizeof(start_daq));

    printf("DAQ setup complete.\n");

    close(sock);
    return 0;
}