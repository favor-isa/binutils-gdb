#ifndef _ELF_FAVOR_H
#define _ELF_FAVOR_H

#include "elf/reloc-macros.h"

START_RELOC_NUMBERS (elf_favor_reloc_type)
  RELOC_NUMBER (R_FAVOR_NONE, 0)
  RELOC_NUMBER (R_FAVOR_J22_PCREL, 1)
END_RELOC_NUMBERS (R_FAVOR_max)

#endif