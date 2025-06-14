// Implements file persistence for session state
#include "state.h"
#include <stdio.h>
#include <string.h>

int load_xcp_state(XcpSessionState *state)
{
    FILE *f = fopen("xcp_packets.bin", "rb");
    if (!f)
        return -1;
    if (fread(state, sizeof(XcpSessionState), 1, f) != 1)
    {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

void save_xcp_state(const XcpSessionState *state)
{
    FILE *f = fopen("xcp_packets.bin", "wb");
    if (!f)
        return;
    fwrite(state, sizeof(XcpSessionState), 1, f);
    fclose(f);
}

// TODO - there is a bug here, if XCP_CMD floods us with packets,
// we will have an ever-growing list of XCP packets in session_state.
// Unclear what the right solution is - throw away old packets?
void add_xcp_packet(XcpSessionState *state, const uint8_t *data, size_t len)
{
    if (state->packet_count >= MAX_XCP_PACKETS || len > MAX_PACKET_SIZE)
        return;

    memcpy(state->packets[state->packet_count], data, len);
    state->packet_lengths[state->packet_count] = len;

    state->packet_count++;
    save_xcp_state(state);
}

void reset_xcp_state(XcpSessionState *state)
{
    memset(state, 0, sizeof(XcpSessionState));
    save_xcp_state(state);
}