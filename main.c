/* ========================================================
 * main.c - Entry point and menu-driven CLI
 *
 * Distributed Tumor Treatment Simulator
 * EGC 301P / Operating Systems Lab – Mini Project
 *
 * OS Concepts Summary:
 *   4.1 Role-Based Authorization  – Admin / Doctor / Patient menus
 *   4.2 File Locking              – pthread_rwlock (data.c / simulation.c)
 *   4.3 Concurrency Control       – pthread_mutex + semaphore (data.c / ipc.c)
 *   4.4 Data Consistency          – mutex-protected read-modify-write, clamping
 *   4.5 Socket Programming        – TCP server in socket_server.c
 *   4.6 IPC                       – Named FIFO + fork in ipc.c
 * ======================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "simulation.h"
#include "ipc.h"
#include "socket_server.h"

/* --------------------------------------------------------
 * Utility helpers
 * -------------------------------------------------------- */

/* Read a trimmed string from stdin; returns 0 on success */
static int read_string(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return -1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

/* Read an integer; returns -1 on parse failure */
static int read_int(int *out) {
    char buf[32];
    if (read_string(buf, sizeof(buf)) < 0) return -1;
    char *end;
    long v = strtol(buf, &end, 10);
    if (end == buf) return -1;
    *out = (int)v;
    return 0;
}

/* ========================================================
 * ADMIN MENU
 * Role-Based Authorization: only admin sees these options
 * ======================================================== */
static void admin_menu(void) {
    int choice;
    do {
        printf("\n====== ADMIN MENU ======\n");
        printf("  1. Add Doctor\n");
        printf("  2. Add Patient\n");
        printf("  3. Assign Patient to Doctor\n");
        printf("  4. View All Doctors\n");
        printf("  5. View All Patients\n");
        printf("  6. Run Simulation Round\n");
        printf("  0. Back\n");
        printf("Choice: ");

        if (read_int(&choice) < 0) { choice = -1; continue; }

        switch (choice) {
        case 1: {
            int id; char name[64];
            printf("Doctor ID: "); read_int(&id);
            printf("Doctor Name: "); read_string(name, sizeof(name));
            add_doctor(id, name);
            break;
        }
        case 2: {
            int id; char name[64];
            printf("Patient ID: "); read_int(&id);
            printf("Patient Name: "); read_string(name, sizeof(name));
            add_patient(id, name);
            break;
        }
        case 3: {
            int pid, did;
            printf("Patient ID: "); read_int(&pid);
            printf("Doctor ID: ");  read_int(&did);
            assign_patient_to_doctor(pid, did);
            break;
        }
        case 4: list_doctors();  break;
        case 5: list_patients(); break;
        case 6: run_simulation_round(); break;
        case 0: break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

/* ========================================================
 * DOCTOR MENU
 * Role-Based Authorization: doctor can only treat patients
 * assigned to them.
 * ======================================================== */
static void doctor_menu(void) {
    int doctor_id;
    printf("Enter Doctor ID: ");
    if (read_int(&doctor_id) < 0) { printf("Invalid ID.\n"); return; }

    Doctor *d = find_doctor(doctor_id);
    if (!d) { printf("[ACCESS DENIED] Doctor ID %d not found.\n", doctor_id); return; }

    printf("Welcome, Dr. %s!\n", d->name);

    int choice;
    do {
        printf("\n====== DOCTOR MENU (Dr. %s) ======\n", d->name);
        printf("  1. View My Patients\n");
        printf("  2. Apply Treatment to a Patient\n");
        printf("  0. Back\n");
        printf("Choice: ");

        if (read_int(&choice) < 0) { choice = -1; continue; }

        switch (choice) {
        case 1:
            list_doctor_patients(doctor_id);
            break;

        case 2: {
            /* Verify the patient belongs to this doctor – RBAC */
            int pid;
            printf("Patient ID: ");
            if (read_int(&pid) < 0) { printf("Invalid.\n"); break; }

            /* Check ownership */
            int owned = 0;
            for (int i = 0; i < d->patient_count; i++)
                if (d->patient_ids[i] == pid) { owned = 1; break; }

            if (!owned) {
                printf("[ACCESS DENIED] Patient %d is not assigned to you.\n", pid);
                break;
            }

            Patient *p = find_patient(pid);
            if (!p) { printf("[ERROR] Patient not found.\n"); break; }

            display_patient_status(p);

            printf("\nTreatment options:\n");
            printf("  1. Chemotherapy (tumor -20, resistance +5, health -5)\n");
            printf("  2. Radiation    (tumor -15, health -3)\n");
            printf("Choice: ");
            int treat;
            if (read_int(&treat) < 0 || (treat != 1 && treat != 2)) {
                printf("Invalid treatment.\n"); break;
            }
            apply_treatment(p, treat);
            break;
        }
        case 0: break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

/* ========================================================
 * PATIENT MENU
 * Role-Based Authorization: patients can only view their
 * own data and set preferences.
 * ======================================================== */
static void patient_menu(void) {
    int patient_id;
    printf("Enter Patient ID: ");
    if (read_int(&patient_id) < 0) { printf("Invalid ID.\n"); return; }

    Patient *p = find_patient(patient_id);
    if (!p) { printf("[ACCESS DENIED] Patient ID %d not found.\n", patient_id); return; }

    printf("Welcome, %s!\n", p->name);

    int choice;
    do {
        printf("\n====== PATIENT MENU (%s) ======\n", p->name);
        printf("  1. View My Status\n");
        printf("  2. Set Tolerance Level\n");
        printf("  3. Request Reduced Treatment\n");
        printf("  0. Back\n");
        printf("Choice: ");

        if (read_int(&choice) < 0) { choice = -1; continue; }

        switch (choice) {
        case 1:
            display_patient_status(p);
            break;

        case 2: {
            printf("Set Tolerance: 1=Low  2=Medium  3=High\nChoice: ");
            int tol;
            if (read_int(&tol) < 0 || tol < 1 || tol > 3) {
                printf("Invalid tolerance.\n"); break;
            }
            pthread_mutex_lock(&data_mutex);
            p->tolerance = tol;
            pthread_mutex_unlock(&data_mutex);
            const char *name = (tol == 1) ? "Low" : (tol == 2) ? "Medium" : "High";
            printf("Tolerance set to %s.\n", name);
            ipc_log("PATIENT: Tolerance updated");
            break;
        }

        case 3:
            pthread_mutex_lock(&data_mutex);
            p->reduce_request = 1;
            pthread_mutex_unlock(&data_mutex);
            printf("Reduced treatment request flagged. Your doctor will be notified.\n");
            ipc_log("PATIENT: Requested reduced dose");
            break;

        case 0: break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

/* ========================================================
 * MAIN – top-level role selection
 * ======================================================== */
int main(void) {
    printf("=========================================\n");
    printf("  Distributed Tumor Treatment Simulator  \n");
    printf("  EGC 301P - Operating Systems Lab       \n");
    printf("=========================================\n");

    /* Start IPC logger (fork + named FIFO) */
    start_logger();

    /* Start socket server (pthread + TCP) */
    start_socket_server();

    ipc_log("SYSTEM: Simulator started");

    int choice;
    do {
        printf("\n====== ROLE SELECT ======\n");
        printf("  1. Admin\n");
        printf("  2. Doctor\n");
        printf("  3. Patient\n");
        printf("  0. Exit\n");
        printf("Choice: ");

        if (read_int(&choice) < 0) { choice = -1; continue; }

        switch (choice) {
        case 1: admin_menu();   break;
        case 2: doctor_menu();  break;
        case 3: patient_menu(); break;
        case 0: printf("Exiting...\n"); break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    /* Cleanup */
    ipc_log("SYSTEM: Simulator shutting down");
    stop_socket_server();
    cleanup_ipc();

    return 0;
}
