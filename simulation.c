/* ========================================================
 * simulation.c - Tumor growth and treatment logic
 *
 * OS Concepts demonstrated:
 *   - Concurrency Control : mutex protects shared patient state
 *   - Data Consistency    : atomic read-modify-write on health
 *                           and tumor_size to avoid dirty reads
 *   - File Locking        : rwlock used during batch round
 * ======================================================== */

#include <stdio.h>
#include <pthread.h>
#include "simulation.h"
#include "data.h"
#include "ipc.h"

/* ========================================================
 * apply_treatment
 *
 * Applies the selected treatment to a patient.
 * Role-Based Access Control is enforced by the caller
 * (only doctors may call this); constraints checked here.
 *
 * Returns  0 on success
 *         -1 if treatment was blocked (safety constraint)
 *         -2 if patient pointer is NULL
 * ======================================================== */
int apply_treatment(Patient *p, int treatment_type) {
    if (!p) return -2;

    pthread_mutex_lock(&data_mutex); /* Exclusive access – no dirty reads */

    /* Constraint: low-tolerance patients with poor health
     * must not receive aggressive (chemo) treatment.        */
    if (treatment_type == TREATMENT_CHEMO &&
        p->health < 50 && p->tolerance == TOLERANCE_LOW) {
        printf("[BLOCKED] Patient %d has low tolerance and health < 50. "
               "Chemotherapy not permitted.\n", p->id);
        pthread_mutex_unlock(&data_mutex);
        return -1;
    }

    /* If patient requested dose reduction, halve the impact */
    int reduction = p->reduce_request ? 2 : 1;

    if (treatment_type == TREATMENT_CHEMO) {
        printf("  Applying Chemotherapy to Patient %d...\n", p->id);
        p->tumor_size      -= 20 / reduction;
        p->resistant_cells += 5;
        p->health          -= 5 / reduction;
        ipc_log("TREATMENT: Chemotherapy applied");
    } else if (treatment_type == TREATMENT_RADIATION) {
        printf("  Applying Radiation to Patient %d...\n", p->id);
        p->tumor_size -= 15 / reduction;
        p->health     -= 3  / reduction;
        ipc_log("TREATMENT: Radiation applied");
    } else {
        printf("[ERROR] Unknown treatment type %d.\n", treatment_type);
        pthread_mutex_unlock(&data_mutex);
        return -1;
    }

    /* Clamp values – tumor_size and health never go below 0 */
    if (p->tumor_size < 0) p->tumor_size = 0;
    if (p->health     < 0) p->health     = 0;

    printf("  [Result] Tumor: %d | Health: %d | Resistance: %d\n",
           p->tumor_size, p->health, p->resistant_cells);

    pthread_mutex_unlock(&data_mutex);
    return 0;
}

/* ========================================================
 * run_simulation_round
 *
 * Advances one round: tumors grow and resistance increases
 * slightly for all active patients.
 * Write-lock prevents reading stale data mid-update.
 * ======================================================== */
void run_simulation_round(void) {
    pthread_rwlock_wrlock(&file_rwlock); /* Write-lock: exclusive batch update */

    printf("\n--- Simulation Round ---\n");
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (!patients[i].active) continue;

        patients[i].tumor_size += 10; /* Tumor grows each round */

        /* Small random-like resistance increase based on existing resistance */
        if (patients[i].resistant_cells > 0)
            patients[i].resistant_cells += 1;

        printf("  Patient [%d] %-20s | Tumor grew to %d | "
               "Resistance: %d | Health: %d\n",
               patients[i].id, patients[i].name,
               patients[i].tumor_size,
               patients[i].resistant_cells,
               patients[i].health);
    }

    pthread_rwlock_unlock(&file_rwlock);
    ipc_log("SIMULATION: Round completed");
}

/* ========================================================
 * display_patient_status
 * ======================================================== */
void display_patient_status(const Patient *p) {
    if (!p) { printf("[ERROR] Patient not found.\n"); return; }

    const char *tol = (p->tolerance == TOLERANCE_LOW)    ? "Low"
                    : (p->tolerance == TOLERANCE_MEDIUM) ? "Medium"
                                                         : "High";
    printf("\n--- Patient Status ---\n");
    printf("  ID            : %d\n",  p->id);
    printf("  Name          : %s\n",  p->name);
    printf("  Tumor Size    : %d\n",  p->tumor_size);
    printf("  Resistant Cells: %d\n", p->resistant_cells);
    printf("  Health        : %d\n",  p->health);
    printf("  Tolerance     : %s\n",  tol);
    printf("  Reduce Request: %s\n",  p->reduce_request ? "Yes" : "No");
    printf("  Assigned Dr.  : %d\n",  p->assigned_doctor);
}
