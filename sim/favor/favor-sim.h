#ifndef FAVOR_SIM_H
#define FAVOR_SIM_H

struct favor_sim_cpu {
    uint64_t gpr[32];

    /* The program counter. */
    sim_cia  pc;
};

#define FAVOR_SIM_CPU(scpu) ((struct favor_sim_cpu*)CPU_ARCH_DATA(scpu))

#endif