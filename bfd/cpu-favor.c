#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

const bfd_arch_info_type bfd_favor_arch = {
    64, /* bits per word */
    64, /* bits per address */
    8,  /* bits per byte */
    bfd_arch_favor, /* arch */
    bfd_mach_favor, /* mach */
    "favor", /* arch name */
    "favor", /* printable name */
    2, /* section align power */
    true, /* default machine for the architecture */
    bfd_default_compatible, /* compatible */
    bfd_default_scan,       /* scan */
    bfd_arch_default_fill,  /* fill */
    NULL, /* next machine */
    0, /* max reloc offset into insn. may need to be updated ..? */
};