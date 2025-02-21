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
#include "target-newlib-syscall.h"

#include "favor-sim.h"

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int signal) /* ignore  */
{
  /* Run instructions here. */
    for(;;) {
        printf("hello world...\n");
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
    return 0;
}

static void
favor_pc_set (sim_cpu *cpu, sim_cia pc)
{
    /* TODO */
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
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

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */

  (void)sd;
  (void)prog_bfd;
  (void)argv;
  (void)env;
  (void)scpu;

  // load the starting address. TODO
  //if (prog_bfd != NULL)
  //  cpu.asregs.regs[PC_REGNO] = bfd_get_start_address (prog_bfd);

      /* Store the string.  */
      //sim_core_write_buffer (sd, scpu, write_map, argv[i],
	//		     tp, strlen(argv[i])+1);

  return SIM_RC_OK;
}
