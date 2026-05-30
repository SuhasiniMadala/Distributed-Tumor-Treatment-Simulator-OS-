/* ========================================================
 * socket_server.c - TCP socket server (client-server model)
 *
 * OS Concept: Socket Programming
 *   - Server binds to SOCK_PORT and accepts connections
 *   - Each client receives a summary of all active patients
 *   - Runs in a dedicated pthread (Concurrency Control)
 *
 * To test:
 *   nc 127.0.0.1 9090
 * ======================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "socket_server.h"
#include "data.h"
#include "types.h"

static int          server_fd   = -1;
static pthread_t    server_tid;
static volatile int server_running = 0;

/* --------------------------------------------------------
 * handle_client: sends patient summary to a connected client
 * -------------------------------------------------------- */
static void handle_client(int client_fd) {
    char buf[4096];
    int  pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "=== Tumor Treatment Simulator – Status Report ===\n");

    pthread_rwlock_rdlock(&file_rwlock); /* Read-lock */
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (!patients[i].active) continue;
        const char *tol = (patients[i].tolerance == TOLERANCE_LOW)    ? "Low"
                        : (patients[i].tolerance == TOLERANCE_MEDIUM) ? "Medium"
                                                                       : "High";
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        "Patient [%d] %-20s | Tumor: %3d | Health: %3d | "
                        "Resistance: %2d | Tolerance: %s\n",
                        patients[i].id, patients[i].name,
                        patients[i].tumor_size, patients[i].health,
                        patients[i].resistant_cells, tol);
    }
    pthread_rwlock_unlock(&file_rwlock);

    if (pos == (int)strlen("=== Tumor Treatment Simulator – Status Report ===\n"))
        pos += snprintf(buf + pos, sizeof(buf) - pos, "(No patients)\n");

    pos += snprintf(buf + pos, sizeof(buf) - pos, "===\n");
    write(client_fd, buf, pos);
    close(client_fd);
}

/* --------------------------------------------------------
 * server_thread: accept loop
 * -------------------------------------------------------- */
static void *server_thread(void *arg) {
    (void)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (server_running) {
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (server_running) perror("accept");
            break;
        }
        handle_client(client_fd); /* Serve synchronously; extend with threads for load */
    }
    return NULL;
}

/* --------------------------------------------------------
 * start_socket_server: binds, listens, spawns thread
 * -------------------------------------------------------- */
void start_socket_server(void) {
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(SOCK_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); server_fd = -1; return;
    }
    if (listen(server_fd, SOCK_BACKLOG) < 0) {
        perror("listen"); close(server_fd); server_fd = -1; return;
    }

    server_running = 1;
    pthread_create(&server_tid, NULL, server_thread, NULL);
    printf("[SOCKET] Status server listening on port %d (use: nc 127.0.0.1 %d)\n",
           SOCK_PORT, SOCK_PORT);
}

/* --------------------------------------------------------
 * stop_socket_server: close fd, join thread
 * -------------------------------------------------------- */
void stop_socket_server(void) {
    server_running = 0;
    if (server_fd >= 0) { close(server_fd); server_fd = -1; }
    pthread_join(server_tid, NULL);
}
