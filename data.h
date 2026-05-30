#ifndef DATA_H
#define DATA_H

/* ========================================================
 * data.h - Patient/Doctor management declarations
 * ======================================================== */

#include "types.h"

/* Global in-memory storage (defined in data.c) */
extern Patient  patients[MAX_PATIENTS];
extern Doctor   doctors[MAX_DOCTORS];
extern int      patient_count;
extern int      doctor_count;

/* Mutex for shared data protection (defined in data.c) */
#include <pthread.h>
extern pthread_mutex_t data_mutex;
extern pthread_rwlock_t file_rwlock; /* File-locking simulation */

/* ---- Doctor management ---- */
int  add_doctor(int id, const char *name);
Doctor *find_doctor(int id);
void list_doctors(void);

/* ---- Patient management ---- */
int  add_patient(int id, const char *name);
Patient *find_patient(int id);
void list_patients(void);
void list_doctor_patients(int doctor_id);

/* ---- Assignment ---- */
int  assign_patient_to_doctor(int patient_id, int doctor_id);

#endif /* DATA_H */
