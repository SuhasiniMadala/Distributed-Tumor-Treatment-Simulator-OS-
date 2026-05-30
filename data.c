/* ========================================================
 * data.c - In-memory patient/doctor management
 *
 * OS Concepts demonstrated:
 *   - Concurrency Control : pthread_mutex protects writes
 *   - Data Consistency    : mutex prevents race conditions /
 *                           lost updates on shared arrays
 *   - File Locking        : pthread_rwlock simulates safe
 *                           read/write access to "records"
 * ======================================================== */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "data.h"

/* ---- Global storage ---- */
Patient  patients[MAX_PATIENTS];
Doctor   doctors[MAX_DOCTORS];
int      patient_count = 0;
int      doctor_count  = 0;

/* ---- Synchronisation primitives ---- */
pthread_mutex_t data_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t file_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/* ========================================================
 * Doctor management
 * ======================================================== */

/* add_doctor: inserts a new doctor record.
 * Returns 0 on success, -1 on failure. */
int add_doctor(int id, const char *name) {
    pthread_mutex_lock(&data_mutex); /* Concurrency Control */

    if (doctor_count >= MAX_DOCTORS) {
        printf("[ERROR] Max doctor capacity reached.\n");
        pthread_mutex_unlock(&data_mutex);
        return -1;
    }
    /* Check for duplicate ID */
    for (int i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].active && doctors[i].id == id) {
            printf("[ERROR] Doctor ID %d already exists.\n", id);
            pthread_mutex_unlock(&data_mutex);
            return -1;
        }
    }
    /* Find free slot */
    for (int i = 0; i < MAX_DOCTORS; i++) {
        if (!doctors[i].active) {
            doctors[i].id            = id;
            doctors[i].patient_count = 0;
            doctors[i].active        = 1;
            strncpy(doctors[i].name, name, 63);
            doctors[i].name[63] = '\0';
            doctor_count++;
            printf("[INFO] Doctor '%s' (ID: %d) added.\n", name, id);
            pthread_mutex_unlock(&data_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&data_mutex);
    return -1;
}

/* find_doctor: returns pointer to doctor with given ID, or NULL */
Doctor *find_doctor(int id) {
    pthread_rwlock_rdlock(&file_rwlock); /* Read-lock: multiple readers safe */
    for (int i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].active && doctors[i].id == id) {
            pthread_rwlock_unlock(&file_rwlock);
            return &doctors[i];
        }
    }
    pthread_rwlock_unlock(&file_rwlock);
    return NULL;
}

/* list_doctors: prints all active doctors and their patients */
void list_doctors(void) {
    pthread_rwlock_rdlock(&file_rwlock);
    printf("\n=== Doctor List ===\n");
    int found = 0;
    for (int i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].active) {
            printf("  Doctor [ID: %d] %s | Patients: %d\n",
                   doctors[i].id, doctors[i].name, doctors[i].patient_count);
            for (int j = 0; j < doctors[i].patient_count; j++) {
                printf("    -> Patient ID: %d\n", doctors[i].patient_ids[j]);
            }
            found = 1;
        }
    }
    if (!found) printf("  (No doctors registered)\n");
    pthread_rwlock_unlock(&file_rwlock);
}

/* ========================================================
 * Patient management
 * ======================================================== */

/* add_patient: inserts a new patient with default values.
 * Returns 0 on success, -1 on failure. */
int add_patient(int id, const char *name) {
    pthread_mutex_lock(&data_mutex); /* Lock before writing – Data Consistency */

    if (patient_count >= MAX_PATIENTS) {
        printf("[ERROR] Max patient capacity reached.\n");
        pthread_mutex_unlock(&data_mutex);
        return -1;
    }
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patients[i].active && patients[i].id == id) {
            printf("[ERROR] Patient ID %d already exists.\n", id);
            pthread_mutex_unlock(&data_mutex);
            return -1;
        }
    }
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (!patients[i].active) {
            patients[i].id               = id;
            patients[i].tumor_size       = 100; /* default initial size */
            patients[i].resistant_cells  = 0;
            patients[i].health           = 100;
            patients[i].tolerance        = TOLERANCE_MEDIUM;
            patients[i].assigned_doctor  = -1;
            patients[i].reduce_request   = 0;
            patients[i].active           = 1;
            strncpy(patients[i].name, name, 63);
            patients[i].name[63] = '\0';
            patient_count++;
            printf("[INFO] Patient '%s' (ID: %d) added.\n", name, id);
            pthread_mutex_unlock(&data_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&data_mutex);
    return -1;
}

/* find_patient: returns pointer to patient with given ID, or NULL */
Patient *find_patient(int id) {
    pthread_rwlock_rdlock(&file_rwlock);
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patients[i].active && patients[i].id == id) {
            pthread_rwlock_unlock(&file_rwlock);
            return &patients[i];
        }
    }
    pthread_rwlock_unlock(&file_rwlock);
    return NULL;
}

/* list_patients: prints all active patients */
void list_patients(void) {
    pthread_rwlock_rdlock(&file_rwlock);
    printf("\n=== Patient List ===\n");
    int found = 0;
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patients[i].active) {
            const char *tol = (patients[i].tolerance == TOLERANCE_LOW)    ? "Low"
                            : (patients[i].tolerance == TOLERANCE_MEDIUM) ? "Medium"
                                                                           : "High";
            printf("  Patient [ID: %d] %-20s | Tumor: %3d | Health: %3d | "
                   "Resistance: %2d | Tolerance: %-6s | Doctor: %d\n",
                   patients[i].id, patients[i].name,
                   patients[i].tumor_size, patients[i].health,
                   patients[i].resistant_cells, tol,
                   patients[i].assigned_doctor);
            found = 1;
        }
    }
    if (!found) printf("  (No patients registered)\n");
    pthread_rwlock_unlock(&file_rwlock);
}

/* list_doctor_patients: prints patients assigned to one doctor */
void list_doctor_patients(int doctor_id) {
    Doctor *d = find_doctor(doctor_id);
    if (!d) { printf("[ERROR] Doctor %d not found.\n", doctor_id); return; }

    pthread_rwlock_rdlock(&file_rwlock);
    printf("\n=== Patients of Doctor %d (%s) ===\n", d->id, d->name);
    if (d->patient_count == 0) {
        printf("  (No patients assigned)\n");
        pthread_rwlock_unlock(&file_rwlock);
        return;
    }
    for (int j = 0; j < d->patient_count; j++) {
        Patient *p = find_patient(d->patient_ids[j]);
        if (p) {
            printf("  [ID: %d] %-20s | Tumor: %3d | Health: %3d | Resistance: %2d\n",
                   p->id, p->name, p->tumor_size, p->health, p->resistant_cells);
        }
    }
    pthread_rwlock_unlock(&file_rwlock);
}

/* ========================================================
 * Assignment
 * ======================================================== */

/* assign_patient_to_doctor: links patient to doctor.
 * Returns 0 on success, -1 on failure. */
int assign_patient_to_doctor(int patient_id, int doctor_id) {
    pthread_mutex_lock(&data_mutex); /* Write-lock: exclusive modification */

    Patient *p = NULL;
    Doctor  *d = NULL;

    for (int i = 0; i < MAX_PATIENTS; i++)
        if (patients[i].active && patients[i].id == patient_id) { p = &patients[i]; break; }
    for (int i = 0; i < MAX_DOCTORS; i++)
        if (doctors[i].active && doctors[i].id == doctor_id)    { d = &doctors[i];  break; }

    if (!p) { printf("[ERROR] Patient %d not found.\n", patient_id); pthread_mutex_unlock(&data_mutex); return -1; }
    if (!d) { printf("[ERROR] Doctor %d not found.\n",  doctor_id);  pthread_mutex_unlock(&data_mutex); return -1; }

    if (d->patient_count >= MAX_PER_DOCTOR) {
        printf("[ERROR] Doctor %d has reached max patient limit.\n", doctor_id);
        pthread_mutex_unlock(&data_mutex);
        return -1;
    }

    /* Remove patient from previous doctor if any */
    if (p->assigned_doctor != -1) {
        for (int i = 0; i < MAX_DOCTORS; i++) {
            if (doctors[i].active && doctors[i].id == p->assigned_doctor) {
                for (int j = 0; j < doctors[i].patient_count; j++) {
                    if (doctors[i].patient_ids[j] == patient_id) {
                        /* Shift remaining entries left */
                        for (int k = j; k < doctors[i].patient_count - 1; k++)
                            doctors[i].patient_ids[k] = doctors[i].patient_ids[k+1];
                        doctors[i].patient_count--;
                        break;
                    }
                }
                break;
            }
        }
    }

    d->patient_ids[d->patient_count++] = patient_id;
    p->assigned_doctor = doctor_id;
    printf("[INFO] Patient %d assigned to Doctor %d.\n", patient_id, doctor_id);

    pthread_mutex_unlock(&data_mutex);
    return 0;
}
