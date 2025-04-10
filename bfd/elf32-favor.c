#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/favor.h"



static reloc_howto_type
favor_elf_howto_table[] = {
    HOWTO(R_FAVOR_NONE,
        0,
        0,
        0,
        false,
        0,
        complain_overflow_dont,
        bfd_elf_generic_reloc,
        "R_FAVOR_NONE",
        false,
        0,
        0,
        false),

    HOWTO(R_FAVOR_J23_PCREL,
        2, // mask off last 2 bits?
        2,
        23,
        true,
        9,
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_J23_PCREL",
        true,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0)
};

#define FAVOR_RELOC_TABLE_SIZE (sizeof(favor_elf_howto_table) / sizeof(*favor_elf_howto_table))

static reloc_howto_type*
favor_elf_reloc_type_lookup(bfd *abfd ATTRIBUTE_UNUSED, bfd_reloc_code_real_type code) {
    if(code == BFD_RELOC_NONE)            return &favor_elf_howto_table[R_FAVOR_NONE];
    if(code == BFD_RELOC_FAVOR_J23_PCREL) return &favor_elf_howto_table[R_FAVOR_J23_PCREL];
    return NULL;
}

static reloc_howto_type*
favor_elf_reloc_name_lookup(bfd *abfd ATTRIBUTE_UNUSED, const char *r_name) {
    for(size_t i = 0; i < FAVOR_RELOC_TABLE_SIZE; ++i) {
        if(!strcasecmp(favor_elf_howto_table[i].name, r_name)) { return &favor_elf_howto_table[i]; }
    }
    return NULL;
}

static bool
favor_elf_info_to_howto(bfd *abfd ATTRIBUTE_UNUSED,
    arelent *cache_ptr,
    Elf_Internal_Rela *dst)
{
    unsigned int r = ELF32_R_TYPE(dst->r_info);
    if (r >= (unsigned int) R_FAVOR_max)
    {
        /* xgettext:c-format */
        _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
                abfd, r);
        bfd_set_error (bfd_error_bad_value);
        return false;
    }
    cache_ptr->howto = &favor_elf_howto_table[r];
    return true;
}

#define TARGET_LITTLE_SYM favor_elf32_vec
#define TARGET_LITTLE_NAME "elf32-favor"
#define ELF_ARCH         bfd_arch_favor
#define ELF_MACHINE_CODE EM_FAVOR
#define ELF_MAXPAGESIZE  1
#define bfd_elf32_bfd_reloc_type_lookup favor_elf_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup favor_elf_reloc_name_lookup
#define elf_info_to_howto               favor_elf_info_to_howto

#include "elf32-target.h"