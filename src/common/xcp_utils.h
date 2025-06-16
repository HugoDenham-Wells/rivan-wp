#ifndef XCP_UTILS_H
#define XCP_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool is_daq_packet(const uint8_t *data, size_t len);

#endif // XCP_UTILS_H