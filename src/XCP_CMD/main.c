#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "networking.h"

int main()
{
    int sock = connect_with_retry(CMD_ADDR, CMD_PORT, "XCP_CMD");
    printf("Connected to CMD at %s:%d\n", CMD_ADDR, CMD_PORT);

    // DAQ setup for SIMPLE_RT_Y.Q_1 and Q_2 (8-byte floats at known addresses)

    // TODO - sending this data does not set up the DAQ properly, we need to
    // fix something... but what?

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

    // TODO - we should probably wait for a response from the CMD before proceeding.
    // For now, we assume the setup is successful and proceed. In a real application,
    // we would handle the response and any errors accordingly.
    printf("DAQ setup complete.\n");

    close(sock);
    return 0;
}