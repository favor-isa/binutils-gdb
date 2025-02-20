#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"

#define TARGET_LITTLE_SYM favor_elf32_vec
#define TARGET_LITTLE_NAME "elf32-favor"
#define ELF_ARCH         bfd_arch_favor
#define ELF_MACHINE_CODE EM_FAVOR
#define ELF_MAXPAGESIZE  1
#define bfd_elf32_bfd_reloc_type_lookup bfd_default_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup _bfd_norelocs_bfd_reloc_name_lookup
#define elf_info_to_howto               _bfd_elf_no_info_to_howto

#include "elf32-target.h"