/* Simulator for the moxie processor
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by Anthony Green

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "bfd.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-signal.h"
#include "sim-syscall.h"
#include "target-newlib-syscall.h"

#include "favor-sim.h"
#include "opcode/favor.h"

static uint32_t
fetch_code(sim_cpu *scpu, struct favor_sim_cpu *cpu) {
    uint32_t code = 0;
    code |= sim_core_read_aligned_1(scpu, cpu->pc, read_map, cpu->pc);
    code |= (sim_core_read_aligned_1(scpu, cpu->pc + 1, read_map, cpu->pc + 1) << 8);
    code |= (sim_core_read_aligned_1(scpu, cpu->pc + 2, read_map, cpu->pc + 2) << 16);
    code |= (sim_core_read_aligned_1(scpu, cpu->pc + 3, read_map, cpu->pc + 3) << 24);
    return code;
}

static bool
opcode_mask(struct favor_sim_cpu *cpu, uint32_t conditional, uint32_t element, uint32_t vec) {
  if(element > vec) return false; // Never execute for elements outsidie of the vector.
  if(!conditional) return true; // Unconditional always execute.
  // Otherwise, predicate based on the individual C flags.
  return cpu->c_codes[element];
}

static uint64_t
sign_bit(uint32_t sz) {
  switch(sz) { 
    case 0: return                0x80ULL;
    case 1: return              0x8000ULL;
    case 2: return          0x80000000ULL;
    default: return 0x8000000000000000ULL;
  }
}

static uint64_t
sign_bit_shift(uint32_t sz) {
  switch(sz) { 
    case 0: return   7;
    case 1: return  15;
    case 2: return  31;
    default: return 63;
  }
}

static bool
compute_condition(struct favor_sim_status *status, int condition) {
  switch(condition) {
    case COND_EQ:  return  status->zero;
    case COND_NE:  return !status->zero;
    case COND_G:   return !status->zero && (status->sign == status->overflow);
    case COND_L:   return  status->sign != status->overflow;
    case COND_GE:  return  status->sign == status->overflow;
    case COND_LE:  return  status->zero && (status->sign != status->overflow);
    case COND_GU:  return  status->carry && !status->zero;
    case COND_LU:  return !status->carry;
    case COND_GEU: return  status->carry;
    case COND_LEU: return !status->carry ||  status->zero;
    case COND_NEG: return  status->sign;
    case COND_POS: return !status->sign;
  }
  return 0;
}

static void
status_int(struct favor_sim_status *status, uint64_t src1, uint64_t src2, uint64_t dest, uint32_t sz) {
  status->zero = (dest == 0);
  status->sign = !!(dest & sign_bit(sz));
  status->carry = 0;
  status->overflow = 0;
}

static void
status_add(struct favor_sim_status *status, uint64_t src1, uint64_t src2, uint64_t dest, uint32_t sz) {
  uint64_t signbit = sign_bit(sz);

  status_int(status, src1, src2, dest, sz);

  // Carry bit is set in the following cases:
  // - Both src1 and src2 have a 1 in the top bit
  // - Either src1 or src2 have a 1 in the top bit, and the result doesn't
  status->carry = ((src1 & signbit) && (src2 & signbit))
    || ((src1 & signbit) && !(dest & signbit))
    || ((src2 & signbit) && !(dest & signbit));

  // Overflow bit is set in the following cases:
  // - The sign bits of the two inputs are the same, and the output bit does
  //   not match.
  status->overflow = ((src1 & signbit) == (src2 & signbit)) && ((src1 & signbit) != (dest & signbit));
}

static void
status_left_shift(struct favor_sim_status *status, uint64_t src1, uint64_t src2, uint64_t dest, uint32_t sz) {
  status_int(status, src1, src2, dest, sz);
  
  // Carry is the last bit shifted out.
  status->carry = !!((src1 << (src2 - 1)) & sign_bit(sz));
}

static void
status_right_shift(struct favor_sim_status *status, uint64_t src1, uint64_t src2, uint64_t dest, uint32_t sz) {
  status_int(status, src1, src2, dest, sz);
  
  // Carry is the last bit shifted out.
  status->carry = !!((src1 >> (src2 - 1)) & 1);
}

// static void
// status1_nop(struct favor_sim_status*, uint64_t, uint64_t) { }

#define APPLY_SINGLE_VEC3(opcode, fn, statusfn, idx, ...) \
  if(opcode_mask(cpu, opcode.conditional, idx, opcode.vec)) { \
    uint64_t _src1 = cpu->gpr[opcode.src1]; \
    uint64_t _src2 = cpu->gpr[opcode.src2]; \
    uint64_t _dest = fn(_src1, _src2, ##__VA_ARGS__); \
    statusfn(&cpu->status[0 | idx], _src1, _src2, _dest, opcode.sz); \
    cpu->gpr[opcode.dest | idx] = _dest; \
  }

#define APPLY_VEC3(opcode, fn, statusfn, ...) do { \
  APPLY_SINGLE_VEC3(opcode, fn, statusfn, 0, __VA_ARGS__) \
  APPLY_SINGLE_VEC3(opcode, fn, statusfn, 1, __VA_ARGS__) \
  APPLY_SINGLE_VEC3(opcode, fn, statusfn, 2, __VA_ARGS__) \
  APPLY_SINGLE_VEC3(opcode, fn, statusfn, 3, __VA_ARGS__) \
} while(0)

#define APPLY_SINGLE_VEC3_X1(opcode, fn, statusfn, idx, ...) \
  if(opcode_mask(cpu, opcode.conditional, idx, opcode.vec)) { \
    uint64_t _in   = cpu->gpr[opcode.dest]; \
    uint64_t _dest = fn(_in, ##__VA_ARGS__); \
    statusfn(&cpu->status[0 | idx], _in, _dest); /* TODO: sz? */ \
    cpu->gpr[opcode.dest | idx] = _dest; \
    TRACE_ALU(scpu, "reg %d <- %lx", (opcode.dest | idx), _dest); \
  }

#define APPLY_VEC3_X1(opcode, fn, statusfn, ...) do { \
  APPLY_SINGLE_VEC3_X1(opcode, fn, statusfn, 0, __VA_ARGS__) \
  APPLY_SINGLE_VEC3_X1(opcode, fn, statusfn, 1, __VA_ARGS__) \
  APPLY_SINGLE_VEC3_X1(opcode, fn, statusfn, 2, __VA_ARGS__) \
  APPLY_SINGLE_VEC3_X1(opcode, fn, statusfn, 3, __VA_ARGS__) \
} while(0)

#define APPLY_SINGLE_VEC_GENERIC(opcode, fn, idx) \
  if(opcode_mask(cpu, opcode.conditional, idx, opcode.vec)) { \
    fn(idx) ; \
  }

#define APPLY_VEC_GENERIC(opcode, fn) do { \
  APPLY_SINGLE_VEC_GENERIC(opcode, fn, 0) \
  APPLY_SINGLE_VEC_GENERIC(opcode, fn, 1) \
  APPLY_SINGLE_VEC_GENERIC(opcode, fn, 2) \
  APPLY_SINGLE_VEC_GENERIC(opcode, fn, 3) \
} while(0)

#define MASKED_ARITH(src1, src2, sz, op) mask_sz(src1 op src2, sz)

static uint64_t
ld_imm(uint64_t in, uint32_t funct, uint64_t imm, uint64_t pc) {
  switch(funct) {
    case LDI3U:   return       (imm << 48ULL); // replaces the value
    case LDI2O:   return (in | (imm << 32ULL)); 
    case LDI1O:   return (in | (imm << 16ULL));
    case LDI0OPC: return (in | (imm <<  0ULL)) + pc;
    default: return in; // TODO: fault?
  }
}

static uint64_t
mask_sz(uint64_t value, uint32_t sz) {
  switch(sz) {
    case 0: return value &       0xFFULL;
    case 1: return value &     0xFFFFULL;
    case 2: return value & 0xFFFFFFFFULL;
    case 3:
    default:
      return value;
  }
}

static uint64_t
mask_szs(int64_t value, uint32_t sz) {
  return mask_sz((uint64_t)value, sz);
}

static int64_t
sextend(uint64_t in, uint32_t sz) {
  switch(sz) {
    case 0: if(in &      0x80ull) { return in |= 0xFFFFFFFFFFFFFF00ull; }; break;
    case 1: if(in &    0x8000ull) { return in |= 0xFFFFFFFFFFFF0000ull; }; break;
    case 2: if(in & 0x8000000ull) { return in |= 0xFFFFFFFF00000000ull; }; break;
  }
  return (int64_t)in;
}

static uint64_t
rshs(uint64_t a, uint64_t b, uint32_t sz) {
  int64_t as = (int64_t)a;
  int64_t bs = (int64_t)b;
  // This is wrong as we need to do the sign extension differently.
  // TODO: Is that a problem for other ops? Do we want to have our registers
  // always sign extended? Doesn't that mess up multiplication?
  return mask_sz((uint64_t)(as >> bs), sz); 
}

static
uint64_t
ror(uint64_t a, uint64_t b, uint32_t sz) {
  uint32_t shift = sign_bit_shift(sz);

  if(b > 64) b = 64;
  
  while(b) {
    a = (a >> 1) | ((a & 1) << shift);

    b -= 1;
  }

  return mask_sz(a, sz);
}

static
uint64_t
rol(uint64_t a, uint64_t b, uint32_t sz) {
  uint32_t signbit = sign_bit(sz);

  if(b > 64) b = 64;

  while(b) {
    a = (a << 1) | !!(a & signbit);

    b -= 1;
  }

  return mask_sz(a, sz);
}

static uint64_t
minu(uint64_t a, uint64_t b, uint32_t sz) { return mask_sz(a < b ? a : b, sz); }
static uint64_t
maxu(uint64_t a, uint64_t b, uint32_t sz) { return mask_sz(a < b ? b : a, sz); }

static uint64_t
mins(uint64_t au, uint64_t bu, uint32_t sz) {
  int64_t a = sextend(au, sz), b = sextend(bu, sz);
  return mask_szs(a < b ? a : b, sz);
}
static uint64_t
maxs(uint64_t au, uint64_t bu, uint32_t sz) {
  int64_t a = sextend(au, sz), b = sextend(bu, sz);
  return mask_szs(a < b ? b : a, sz);
}

static int32_t
sign_extend_32(uint32_t input, uint32_t bit) {
    if(input & (1 << bit)) {
        while(bit < (sizeof(input) * 8)) {
            input |= ((uint32_t)1 << bit);
            bit += 1;
        }
    }
    return (int32_t)input;
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int signal) /* ignore  */
{
    sim_cpu *scpu = STATE_CPU(sd, 0);
    struct favor_sim_cpu *cpu = scpu->arch_data;

  /* Run instructions here. */
    for(;;) {
        uint32_t op = fetch_code(scpu, cpu);
        struct insn insn = favor_decode(op);
        void *pc_addr = (void*)cpu->pc;
        bool do_increment_pc = true;

        TRACE_EXTRACT(scpu, "%p: %#08x", pc_addr, op);

        switch(insn.p_opcode) {
            case OP_MISC:
                if(insn.misc.funct >= CCM_SETEQ && insn.misc.funct <= CCM_SETNEG) {
                  int condition = insn.misc.funct - CCM_SETEQ;
                  #define FN(idx) cpu->c_codes[idx] = compute_condition(&cpu->status[idx], condition);
                  APPLY_VEC_GENERIC(insn.misc, FN);
                  #undef FN

                  break;
                }
                //TRACE_DECODE(scpu, "%p: %c cc_misc %#x %#08x", pc_addr, (insn.cc_misc.conditional ? 'c' : 'u'), insn.cc_misc., insn.cc_misc.);

                switch(insn.misc.funct) {
                    case CCM_HALT: {
                        TRACE_INSN(scpu, "%p: halt", pc_addr);
                        /* Done. sigrc param is exit code. Maybe put a0 there? */
                        sim_engine_halt(sd, scpu, NULL, cpu->pc, sim_exited, 0);
                        break;
                    case CCM_SYSCALL:
                        TRACE_INSN(scpu, "%p: syscall", pc_addr);
                        /* TODO: Truncate the values in a well-defined way. */
                        cpu->gpr[REG_A0] = sim_syscall(scpu,
                            (int)cpu->gpr[REG_A11],
                            (long)cpu->gpr[REG_A0],
                            (long)cpu->gpr[REG_A1],
                            (long)cpu->gpr[REG_A2],
                            (long)cpu->gpr[REG_A3]);
                        break;
                    case CCM_NOP: break; /* nop */
                    default:
                        /* Illegal instruction. SIGILL */
                        TRACE_INSN(scpu, "%p: illegal %#x", pc_addr, op);
                        sim_engine_halt(sd, scpu, NULL, cpu->pc, sim_stopped, SIGILL);
                        break;
                  }
                }
                break;
            case POP_I3:
                switch(insn.int3.funct) {
                  case I3_ADD: APPLY_VEC3(insn.int3, MASKED_ARITH, status_add, insn.int3.sz, +); break;
                  case I3_SUB: APPLY_VEC3(insn.int3, MASKED_ARITH, status_add, insn.int3.sz, -); break;
                  case I3_LSH: APPLY_VEC3(insn.int3, MASKED_ARITH, status_left_shift, insn.int3.sz, <<); break;
                  case I3_RSHU: APPLY_VEC3(insn.int3, MASKED_ARITH, status_right_shift, insn.int3.sz, >>); break;
                  case I3_RSHS: APPLY_VEC3(insn.int3, rshs, status_right_shift, insn.int3.sz); break;
                  // TODO: rol, ror
                  case I3_ROL: APPLY_VEC3(insn.int3, rol, status_left_shift, insn.int3.sz); break;
                  case I3_ROR: APPLY_VEC3(insn.int3, ror, status_right_shift, insn.int3.sz); break;
                  case I3_AND: APPLY_VEC3(insn.int3, MASKED_ARITH, status_int, insn.int3.sz, &); break;
                  case I3_OR: APPLY_VEC3(insn.int3, MASKED_ARITH, status_int, insn.int3.sz, |); break;
                  case I3_XOR: APPLY_VEC3(insn.int3, MASKED_ARITH, status_int, insn.int3.sz, ^); break;
                  case I3_MINU: APPLY_VEC3(insn.int3, minu, status_int, insn.int3.sz); break;
                  case I3_MINS: APPLY_VEC3(insn.int3, mins, status_int, insn.int3.sz); break;
                  case I3_MAXU: APPLY_VEC3(insn.int3, maxu, status_int, insn.int3.sz); break;
                  case I3_MAXS: APPLY_VEC3(insn.int3, maxs, status_int, insn.int3.sz); break;
                }
                break;
            case OP_JUMP: {
              int32_t offset = sign_extend_32(insn.jump.immediate, 21) * 4;
              TRACE_DECODE(scpu, "%p: OP_JUMP %d", pc_addr, offset);
              if(insn.jump.funct == J_JUMP) {
                CPU_PC_SET(scpu, cpu->pc + offset);
                do_increment_pc = false;
              }
              break;
            }
            // TODO: Switch to psuedo-ops?
            case POP_LD_IMM: {
              cpu->gpr[insn.ld_imm.dest] = ld_imm(cpu->gpr[insn.ld_imm.dest], insn.ld_imm.funct, insn.ld_imm.imm, cpu->pc);
              TRACE_ALU(scpu, "%s <- %lx", favor_reg_table[insn.ld_imm.dest].name, cpu->gpr[insn.ld_imm.dest]);
              //APPLY_VEC3_X1(insn.ld_imm, ld_imm, status1_nop, insn.ld_imm.funct, insn.ld_imm.imm, cpu->pc);
            }
            default:
                break;
        }

        // Increase pc.
        if(do_increment_pc) {
          CPU_PC_SET(scpu, cpu->pc + 4);
        }

        cpu->gpr[REG_ZERO] = 0;

        // Necessary to e.g. kill the program.
        if (sim_events_tick (sd)) sim_events_process (sd);
    }
}

static int
favor_reg_store (SIM_CPU *scpu, int rn, const void *memory, int length)
{
    return 0;
}

static int
favor_reg_fetch (SIM_CPU *scpu, int rn, void *memory, int length)
{
    return 0;
}

static sim_cia
favor_pc_get (sim_cpu *cpu)
{
    return FAVOR_SIM_CPU(cpu)->pc;
}

static void
favor_pc_set (sim_cpu *cpu, sim_cia pc)
{
    FAVOR_SIM_CPU(cpu)->pc = pc;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* 1GB */
#define DEFAULT_MEM_SIZE (1024 * 1024 * 1024)

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
  uint32_t buf;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct favor_sim_cpu))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = favor_reg_fetch;
      CPU_REG_STORE (cpu) = favor_reg_store;
      CPU_PC_FETCH (cpu) = favor_pc_get;
      CPU_PC_STORE (cpu) = favor_pc_set;

        // TODO: Reset register state.
    }

    if (sim_core_read_buffer (sd, NULL, read_map, &buf, 4, 1) == 0)
        sim_do_commandf (sd, "memory-size %#x", DEFAULT_MEM_SIZE);

    return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */
  struct favor_sim_cpu *cpu = scpu->arch_data;

  (void)sd;
  (void)prog_bfd;
  (void)argv;
  (void)env;
  (void)scpu;

  //printf("sim create inferior called.\n");

  // load the starting address. TODO
  if (prog_bfd != NULL) {
    cpu->pc = bfd_get_start_address(prog_bfd);
    //printf("set pc to start address @ %lx\n", cpu->pc);
  }

  /* Test setup for syscall. */

  return SIM_RC_OK;
}
