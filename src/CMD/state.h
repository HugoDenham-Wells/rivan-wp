// Persistent session state interface
#ifndef STATE_H
#define STATE_H

#include <stddef.h>
#include <stdint.h>

#define MAX_XCP_PACKETS 256
#define MAX_PACKET_SIZE 256

typedef struct
{
    uint8_t packets[MAX_XCP_PACKETS][MAX_PACKET_SIZE];
    size_t packet_lengths[MAX_XCP_PACKETS];
    int packet_count;
} XcpSessionState;

int load_xcp_state(XcpSessionState *state, const char *filename);
void save_xcp_state(const XcpSessionState *state, const char *filename);
void add_xcp_packet(XcpSessionState *state, const uint8_t *data, size_t len, const char *filename);
void reset_xcp_state(XcpSessionState *state, const char *filename);

#endif