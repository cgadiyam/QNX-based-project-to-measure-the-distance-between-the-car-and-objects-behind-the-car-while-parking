#ifndef GLOBAL_H
#define GLOBAL_H
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <hw/inout.h>
#include <stdint.h>

unsigned int Min_Dist, Max_Dist;
uintptr_t porta_data;
uintptr_t portb_data;
uintptr_t ctrl_handle;
unsigned int Num_Captures;
typedef struct timespec timespec;

int privity_err;

#endif
