#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdint.h>
#include <stddef.h>

#define CMD_ADDR "127.0.0.1"
#define CMD_PORT 17726
#define RT_EXE_PORT 17725
#define RT_EXE_ADDR "127.0.0.1" // Change if RT_EXE is remote

/**
 * @brief Connect to a TCP server with blocking retry logic.
 * @param host The hostname or IP address of the server.
 * @param port The port number of the server.
 * @param name The name of the client, optional. Sent upon connection for
 * identification with the server.
 * @return A valid socket file descriptor on success, or -1 on failure.
 * @note This function will keep trying to connect until successful,
 * with a delay between attempts, i.e. it is BLOCKING and can cause a deadlock.
 * TODO - consider a timeout or non-blocking mode to avoid deadlocks.
 */
int connect_with_retry(const char *host, int port, const char *name);

/**
 * @brief Send a packet of data over a TCP socket.
 * @param sock The socket file descriptor to send the data on.
 * @param data Pointer to the data to send.
 * @param len Length of the data in bytes.
 * @note This function will print the sent data in hex format for debugging.
 */
void send_packet(int sock, const uint8_t *data, size_t len);

#endif // NETWORKING_H