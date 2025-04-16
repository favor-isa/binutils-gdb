#ifndef FAVOR_SIM_H
#define FAVOR_SIM_H

struct favor_sim_status {
    uint32_t zero : 1;
    uint32_t carry : 1;
    uint32_t sign : 1;
    uint32_t overflow : 1;
};

struct favor_sim_cpu {
    uint64_t gpr[32];

    bool c_codes[4];

    struct favor_sim_status status[4];

    /* The program counter. */
    sim_cia  pc;
};

#define FAVOR_SIM_CPU(scpu) ((struct favor_sim_cpu*)CPU_ARCH_DATA(scpu))

#endif