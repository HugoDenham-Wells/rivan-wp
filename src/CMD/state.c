// Implements file persistence for session state
#include "state.h"
#include <stdio.h>

int load_state(XcpSessionState *state)
{
    FILE *f = fopen("xcp_state.bin", "rb");
    if (!f)
        return -1;
    if (fread(state, sizeof(XcpSessionState), 1, f) != 1)
    {
        fclose(f);
        return -1; // Error reading state TODO - try and recover or ensure validity of state data via CRC / more robust method.
    }
    fclose(f);
    return 0;
}

void save_state(const XcpSessionState *state)
{
    FILE *f = fopen("xcp_state.bin", "wb");
    fwrite(state, sizeof(XcpSessionState), 1, f);
    fclose(f);
}
