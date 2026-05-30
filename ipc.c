/* ========================================================
 * ipc.c - Inter-Process Communication via named pipe (FIFO)
 *
 * OS Concepts demonstrated:
 *   - IPC              : named pipe (FIFO) between parent
 *                        (main simulator) and child (logger)
 *   - Concurrency      : semaphore protects pipe writes
 *   - Socket-like model: server writes, child process reads
 * ======================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include "ipc.h"

/* File descriptor for the write-end of the FIFO */
static int    fifo_fd  = -1;
static pid_t  child_pid = -1;

/* Named semaphore to serialise pipe writes (Concurrency Control) */
static sem_t *pipe_sem = NULL;
#define SEM_NAME "/tumor_sim_sem"

/* --------------------------------------------------------
 * logger_process: runs in child, reads from FIFO and
 * writes timestamped entries to a log file.
 * -------------------------------------------------------- */
static void logger_process(void) {
    int rfd = open(LOG_FIFO_PATH, O_RDONLY);
    if (rfd < 0) { perror("logger: open FIFO"); exit(1); }

    FILE *logf = fopen("/tmp/tumor_sim.log", "a");
    if (!logf) logf = stderr;

    fprintf(logf, "[LOGGER] Started. Listening on FIFO...\n");
    fflush(logf);

    char buf[256];
    ssize_t n;
    while ((n = read(rfd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        /* Trim trailing newline for clean output */
        if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';

        time_t now = time(NULL);
        char ts[32];
        strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&now));
        fprintf(logf, "[%s] %s\n", ts, buf);
        fflush(logf);

        /* Sentinel: parent signals shutdown */
        if (strcmp(buf, "SHUTDOWN") == 0) break;
    }

    fprintf(logf, "[LOGGER] Shutting down.\n");
    if (logf != stderr) fclose(logf);
    close(rfd);
    exit(0);
}

/* --------------------------------------------------------
 * start_logger: creates FIFO, forks child logger.
 * Returns child PID, or -1 on failure.
 * -------------------------------------------------------- */
pid_t start_logger(void) {
    /* Create named pipe (FIFO) – IPC mechanism */
    unlink(LOG_FIFO_PATH); /* Remove stale FIFO */
    if (mkfifo(LOG_FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }

    /* Named semaphore for serialised writes */
    sem_unlink(SEM_NAME);
    pipe_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (pipe_sem == SEM_FAILED) {
        perror("sem_open");
        pipe_sem = NULL;
    }

    child_pid = fork(); /* Fork child process */
    if (child_pid < 0) { perror("fork"); return -1; }
    if (child_pid == 0) {
        /* Child: becomes the logger */
        logger_process(); /* never returns */
    }

    /* Parent: open write-end of FIFO (blocks until child opens read-end) */
    fifo_fd = open(LOG_FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) { perror("open FIFO (write)"); return -1; }

    printf("[IPC] Logger process started (PID %d). Log: /tmp/tumor_sim.log\n", child_pid);
    return child_pid;
}

/* --------------------------------------------------------
 * ipc_log: sends a message through the named pipe.
 * Semaphore prevents concurrent writes (Data Consistency).
 * -------------------------------------------------------- */
void ipc_log(const char *msg) {
    if (fifo_fd < 0 || !msg) return;

    /* Acquire semaphore before writing – Concurrency Control */
    if (pipe_sem) sem_wait(pipe_sem);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    write(fifo_fd, buf, strlen(buf));

    if (pipe_sem) sem_post(pipe_sem);
}

/* --------------------------------------------------------
 * cleanup_ipc: sends shutdown signal and waits for child.
 * -------------------------------------------------------- */
void cleanup_ipc(void) {
    ipc_log("SHUTDOWN");

    if (fifo_fd >= 0) { close(fifo_fd); fifo_fd = -1; }
    if (child_pid > 0) { waitpid(child_pid, NULL, 0); child_pid = -1; }

    if (pipe_sem) {
        sem_close(pipe_sem);
        sem_unlink(SEM_NAME);
        pipe_sem = NULL;
    }
    unlink(LOG_FIFO_PATH);
    printf("[IPC] Logger stopped. Check /tmp/tumor_sim.log for activity log.\n");
}
