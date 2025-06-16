#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "networking.h"

// --- XCP Command Codes ---
#define CMD_CONNECT 0xFF
#define CMD_DISCONNECT 0xFE
#define CMD_GET_STATUS 0xFD
#define CMD_FREE_DAQ 0x0E // Clears all DAQ lists
#define CMD_ALLOC_DAQ 0x00
#define CMD_ALLOC_ODT 0x01
#define CMD_ALLOC_ODT_ENTRY 0x02
#define CMD_SET_DAQ_PTR 0x03
#define CMD_WRITE_DAQ 0x04
#define CMD_SET_DAQ_LIST_MODE 0x07
#define CMD_START_STOP_DAQ 0x08
#define CMD_START_STOP_SYNC 0x0A

// --- XCP Data Types ---
// For FLOAT64_IEEE, XCP uses DAQ_DTYPE_DOUBLE
#define DAQ_DTYPE_DOUBLE 0x08

// --- XCP DAQ Modes/Parameters ---
#define DAQ_MODE_DAQ 0x00        // Event-driven DAQ
#define DAQ_MODE_BYTE_ORDER 0x00 // Assuming little-endian byte order for addresses in WRITE_DAQ
#define START_DAQ_LIST 0x01      // For SET_DAQ_LIST_MODE: Start a specific DAQ list
#define STOP_DAQ_LIST 0x00       // For SET_DAQ_LIST_MODE: Stop a specific DAQ list
#define START_DAQ_ALL 0x01       // For START_STOP_DAQ: Start all DAQ lists
#define STOP_DAQ_ALL 0x00        // For START_STOP_DAQ: Stop all DAQ lists

// --- Helper function to print command name ---
void print_command_name(const char *name, size_t len)
{
    printf("  --- %s (%zu bytes) ---\n", name, len);
}

int main()
{
    int sock = connect_with_retry(CMD_ADDR, CMD_PORT, "XCP_CMD");
    printf("Connected to CMD at %s:%d\n", CMD_ADDR, CMD_PORT);

    // --- 1. CONNECT Command ---
    // Connects to the XCP slave.
    // Format: [CMD_CONNECT] [mode]
    // Mode 0x00 indicates normal operation mode.
    uint8_t cmd_connect[] = {
        CMD_CONNECT, // Command code (0xFF)
        0x00         // Mode (0x00 for normal)
    };
    print_command_name("CONNECT", sizeof(cmd_connect));
    send_packet(sock, cmd_connect, sizeof(cmd_connect));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 2. FREE_DAQ Command ---
    // Clears all existing DAQ lists on the slave.
    // This is good practice to ensure a clean state before configuring new measurements.
    // Format: [CMD_FREE_DAQ]
    uint8_t cmd_free_daq[] = {
        CMD_FREE_DAQ // Command code (0x0E)
    };
    print_command_name("FREE_DAQ", sizeof(cmd_free_daq));
    send_packet(sock, cmd_free_daq, sizeof(cmd_free_daq));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 3. ALLOC_DAQ Command ---
    // Allocates DAQ lists on the slave. We need one DAQ list.
    // Format: [CMD_ALLOC_DAQ] [number_of_daq_lists]
    uint8_t cmd_alloc_daq[] = {
        CMD_ALLOC_DAQ, // Command code (0x00)
        0x01           // Allocate 1 DAQ list
    };
    print_command_name("ALLOC_DAQ (1 list)", sizeof(cmd_alloc_daq));
    send_packet(sock, cmd_alloc_daq, sizeof(cmd_alloc_daq));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 4. ALLOC_ODT Command ---
    // Allocates ODTs (Object Description Tables) within a specified DAQ list.
    // We allocate 1 ODT in DAQ list 0.
    // Format: [CMD_ALLOC_ODT] [daq_list_number] [number_of_odts]
    uint8_t cmd_alloc_odt[] = {
        CMD_ALLOC_ODT, // Command code (0x01)
        0x00,          // DAQ list number 0
        0x01           // Allocate 1 ODT in DAQ list 0
    };
    print_command_name("ALLOC_ODT (1 ODT in list 0)", sizeof(cmd_alloc_odt));
    send_packet(sock, cmd_alloc_odt, sizeof(cmd_alloc_odt));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 5. ALLOC_ODT_ENTRY Command ---
    // Allocates entries within a specified ODT.
    // We need 2 entries for our two measurements in DAQ list 0, ODT 0.
    // Format: [CMD_ALLOC_ODT_ENTRY] [daq_list_number] [odt_number] [number_of_entries]
    uint8_t cmd_alloc_odt_entry[] = {
        CMD_ALLOC_ODT_ENTRY, // Command code (0x02)
        0x00,                // DAQ list number 0
        0x00,                // ODT number 0
        0x02                 // Allocate 2 entries in DAQ list 0, ODT 0
    };
    print_command_name("ALLOC_ODT_ENTRY (2 entries in list 0, ODT 0)", sizeof(cmd_alloc_odt_entry));
    send_packet(sock, cmd_alloc_odt_entry, sizeof(cmd_alloc_odt_entry));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 6. Configuration for SIMPLE_RT_Y.Q_1 (0x000305B0) ---

    // SET_DAQ_PTR: Points to the specific ODT entry where the next measurement will be written.
    // Format: [CMD_SET_DAQ_PTR] [daq_list_number] [odt_number] [odt_entry_address]
    uint8_t cmd_set_daq_ptr_q1[] = {
        CMD_SET_DAQ_PTR, // Command code (0x03)
        0x00,            // DAQ list number 0
        0x00,            // ODT number 0
        0x00             // ODT entry address 0 (first entry)
    };
    print_command_name("SET_DAQ_PTR (Q_1)", sizeof(cmd_set_daq_ptr_q1));
    send_packet(sock, cmd_set_daq_ptr_q1, sizeof(cmd_set_daq_ptr_q1));
    // usleep(100000); // Simulate network delay and wait for response

    // WRITE_DAQ: Writes the measurement variable's address and data type to the current ODT entry.
    // ECU_ADDRESS: 0x000305B0 (Little-Endian: 0xB0 0x05 0x03 0x00)
    // Data type: FLOAT64_IEEE (XCP DAQ_DTYPE_DOUBLE = 0x08)
    // Format: [CMD_WRITE_DAQ] [daq_mode] [addr_ext] [address (4 bytes)] [data_type]
    uint8_t cmd_write_daq_q1[] = {
        CMD_WRITE_DAQ,          // Command code (0x04)
        0x00,                   // DAQ mode (0x00 for event-driven, or specific settings like DAQ_MODE_BYTE_ORDER if needed)
        0x00,                   // Address extension (typically 0x00)
        0xB0, 0x05, 0x03, 0x00, // ECU Address 0x000305B0 (Little-Endian)
        DAQ_DTYPE_DOUBLE        // Data type (0x08 for FLOAT64_IEEE)
    };
    print_command_name("WRITE_DAQ (Q_1)", sizeof(cmd_write_daq_q1));
    send_packet(sock, cmd_write_daq_q1, sizeof(cmd_write_daq_q1));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 7. Configuration for SIMPLE_RT_Y.Q_2 (0x000305B8) ---

    // SET_DAQ_PTR: Points to the next ODT entry.
    // Format: [CMD_SET_DAQ_PTR] [daq_list_number] [odt_number] [odt_entry_address]
    uint8_t cmd_set_daq_ptr_q2[] = {
        CMD_SET_DAQ_PTR, // Command code (0x03)
        0x00,            // DAQ list number 0
        0x00,            // ODT number 0
        0x01             // ODT entry address 1 (second entry)
    };
    print_command_name("SET_DAQ_PTR (Q_2)", sizeof(cmd_set_daq_ptr_q2));
    send_packet(sock, cmd_set_daq_ptr_q2, sizeof(cmd_set_daq_ptr_q2));
    // usleep(100000); // Simulate network delay and wait for response

    // WRITE_DAQ: Writes the measurement variable's address and data type.
    // ECU_ADDRESS: 0x000305B8 (Little-Endian: 0xB8 0x05 0x03 0x00)
    // Data type: FLOAT64_IEEE (XCP DAQ_DTYPE_DOUBLE = 0x08)
    // Format: [CMD_WRITE_DAQ] [daq_mode] [addr_ext] [address (4 bytes)] [data_type]
    uint8_t cmd_write_daq_q2[] = {
        CMD_WRITE_DAQ,          // Command code (0x04)
        0x00,                   // DAQ mode
        0x00,                   // Address extension
        0xB8, 0x05, 0x03, 0x00, // ECU Address 0x000305B8 (Little-Endian)
        DAQ_DTYPE_DOUBLE        // Data type (0x08 for FLOAT64_IEEE)
    };
    print_command_name("WRITE_DAQ (Q_2)", sizeof(cmd_write_daq_q2));
    send_packet(sock, cmd_write_daq_q2, sizeof(cmd_write_daq_q2));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 8. SET_DAQ_LIST_MODE Command ---
    // Configures and enables the specified DAQ list.
    // Mode: START_DAQ_LIST (0x01) to enable measurements on this list.
    // Event Channel: 0x00 (assuming a default or continuous event trigger)
    // Cycle Time Factor: 0x0000 (for event-driven, or specific cycles if cyclic DAQ)
    // Format: [CMD_SET_DAQ_LIST_MODE] [mode] [daq_list_number] [event_channel] [cycle_time_factor (2 bytes)]
    uint8_t cmd_set_list_mode[] = {
        CMD_SET_DAQ_LIST_MODE, // Command code (0x07)
        START_DAQ_LIST,        // Mode (0x01 to start this DAQ list)
        0x00,                  // DAQ list number 0
        0x00,                  // Event channel (0x00 for generic/default)
        0x00, 0x00             // Cycle time factor (0 for event-driven)
    };
    print_command_name("SET_DAQ_LIST_MODE (start list 0)", sizeof(cmd_set_list_mode));
    send_packet(sock, cmd_set_list_mode, sizeof(cmd_set_list_mode));
    // usleep(100000); // Simulate network delay and wait for response

    // --- 9. START_STOP_DAQ Command ---
    // Globally starts or stops all configured DAQ lists.
    // Mode: START_DAQ_ALL (0x01) to begin the DAQ measurement.
    // Format: [CMD_START_STOP_DAQ] [mode]
    uint8_t cmd_start_daq_all[] = {
        CMD_START_STOP_DAQ, // Command code (0x08)
        START_DAQ_ALL       // Mode (0x01 to start all DAQ lists)
    };
    print_command_name("START_STOP_DAQ (start all)", sizeof(cmd_start_daq_all));
    send_packet(sock, cmd_start_daq_all, sizeof(cmd_start_daq_all));

    // TODO - we should probably wait for a response from the CMD before proceeding.
    // For now, we assume the setup is successful and proceed. In a real application,
    // we would handle the response and any errors accordingly.
    printf("DAQ setup complete.\n");

    close(sock);
    return 0;
}