#ifndef TYPES_H
#define TYPES_H

/* ========================================================
 * types.h - Core data structures and shared constants
 * ======================================================== */

#define MAX_PATIENTS    50
#define MAX_DOCTORS     20
#define MAX_PER_DOCTOR  10

/* Tolerance levels for patients */
#define TOLERANCE_LOW    1
#define TOLERANCE_MEDIUM 2
#define TOLERANCE_HIGH   3

/* --------------------------------------------------------
 * Patient structure
 * -------------------------------------------------------- */
typedef struct {
    int  id;
    int  tumor_size;       /* Current tumor size (never below 0) */
    int  resistant_cells;  /* Cells resistant to treatment       */
    int  health;           /* Overall health score (0–100)       */
    int  tolerance;        /* 1=low, 2=medium, 3=high            */
    int  assigned_doctor;  /* Doctor ID, -1 if unassigned        */
    int  reduce_request;   /* 1 = patient requested reduced dose  */
    int  active;           /* 1 = record in use                  */
    char name[64];
} Patient;

/* --------------------------------------------------------
 * Doctor structure
 * -------------------------------------------------------- */
typedef struct {
    int id;
    int patient_ids[MAX_PER_DOCTOR];
    int patient_count;
    int active;            /* 1 = record in use */
    char name[64];
} Doctor;

#endif /* TYPES_H */
