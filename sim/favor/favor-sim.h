#ifndef FAVOR_SIM_H
#define FAVOR_SIM_H

// TODO: Are we allowed to do this?
#include <stdint.h>

struct favor_sim_cpu {
    uint64_t gpr[32];
};

#endif