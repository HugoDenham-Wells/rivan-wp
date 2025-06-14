// Persistent session state interface
#ifndef STATE_H
#define STATE_H
#include <stdint.h>
#include <stddef.h>

// Holds configuration/state of XCP_CMD session
// TODO - what should actually be in here? Need more details on XCP
typedef struct
{
    int session_active;
    uint8_t last_config[256];
    size_t config_size;
} XcpSessionState;

int load_state(XcpSessionState *state);
void save_state(const XcpSessionState *state);

#endif