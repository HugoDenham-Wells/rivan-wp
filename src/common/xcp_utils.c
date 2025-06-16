#include <stdbool.h>

#include "xcp_utils.h"

bool is_daq_packet(const uint8_t *data, size_t len)
{
    // TODO - implement a proper check for DAQ packets.
    // For now all packets are considered DAQ packets.
    return true;
}