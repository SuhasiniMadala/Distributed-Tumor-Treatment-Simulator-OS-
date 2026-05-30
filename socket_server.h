#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

/* ========================================================
 * socket_server.h - Simple TCP socket server declarations
 *
 * OS Concept: Socket Programming (client-server model)
 * The server listens on a local port. Clients can connect
 * and query patient stats in real time.
 * ======================================================== */

#define SOCK_PORT  9090
#define SOCK_BACKLOG 5

/* Start the socket server in a background thread */
void start_socket_server(void);

/* Stop the socket server */
void stop_socket_server(void);

#endif /* SOCKET_SERVER_H */
