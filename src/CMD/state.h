// Persistent session state interface
#ifndef STATE_H
#define STATE_H

#include <stddef.h>
#include <stdint.h>

#define MAX_XCP_PACKETS 16
#define MAX_PACKET_SIZE 256

typedef struct
{
    uint8_t packets[MAX_XCP_PACKETS][MAX_PACKET_SIZE];
    size_t packet_lengths[MAX_XCP_PACKETS];
    int packet_count;
    int daq_started; // flag for whether the DAQ has been started
} XcpSessionState;

int load_xcp_state(XcpSessionState *state);
void save_xcp_state(const XcpSessionState *state);
void add_xcp_packet(XcpSessionState *state, const uint8_t *data, size_t len);
void reset_xcp_state(XcpSessionState *state);

#endif