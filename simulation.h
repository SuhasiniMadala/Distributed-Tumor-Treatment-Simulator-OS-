#ifndef SIMULATION_H
#define SIMULATION_H

/* ========================================================
 * simulation.h - Tumor growth and treatment declarations
 * ======================================================== */

#include "types.h"

/* Treatment identifiers */
#define TREATMENT_CHEMO     1
#define TREATMENT_RADIATION 2

/* ---- Treatment ---- */
int apply_treatment(Patient *p, int treatment_type);

/* ---- Simulation round ---- */
void run_simulation_round(void);

/* ---- Display ---- */
void display_patient_status(const Patient *p);

#endif /* SIMULATION_H */
