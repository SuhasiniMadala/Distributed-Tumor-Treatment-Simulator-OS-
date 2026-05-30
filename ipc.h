#ifndef IPC_H
#define IPC_H

/* ========================================================
 * ipc.h - Inter-Process Communication declarations
 *
 * Uses POSIX named pipes (FIFOs) as the primary IPC
 * mechanism between the main server process and a
 * logger/monitor child process.
 * ======================================================== */

#define LOG_FIFO_PATH "/tmp/tumor_sim_log.fifo"

/* Start the logger child process; returns child PID */
pid_t start_logger(void);

/* Send a message through the IPC pipe */
void ipc_log(const char *msg);

/* Clean up IPC resources */
void cleanup_ipc(void);

#endif /* IPC_H */
