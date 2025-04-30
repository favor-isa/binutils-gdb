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

    HOWTO(R_FAVOR_J22_PCREL,
        2, // mask off last 2 bits?
        4,
        22,
        true,
        6,
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_J22_PCREL",
        false,
        0x00000000,
        0x0FFFFFC0,
        true),

    HOWTO(R_FAVOR_IMM16_PCREL,
        0, // no shift
        4, // 4 bytes
        64, // 64 bit relocation
        true,
        12, // Left shift by 10. WARNING: We may need a custom relocation function
            // if we change the thing to not be contiguous.
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_IMM16_PCREL",
        false,
        0x00000000,
        0x0FFFF000,
        true),

    HOWTO(R_FAVOR_IMM32_PCREL,
        16, // the rest will shift down part of the input
        4, // 4 bytes
        64, // 64 bit relocation
        true,
        12, // Left shift by 10. WARNING: We may need a custom relocation function
            // if we change the thing to not be contiguous.
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_IMM32_PCREL",
        false,
        0x00000000,
        0x0FFFF000,
        true),

    HOWTO(R_FAVOR_IMM48_PCREL,
        32, // the rest will shift down part of the input
        4, // 4 bytes
        64, // 64 bit relocation
        true,
        12, // Left shift by 10. WARNING: We may need a custom relocation function
            // if we change the thing to not be contiguous.
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_IMM48_PCREL",
        false,
        0x00000000,
        0x0FFFF000,
        true),

    HOWTO(R_FAVOR_IMM64_PCREL,
        48, // the rest will shift down part of the input
        4, // 4 bytes
        64, // 64 bit relocation
        true,
        12, // Left shift by 10. WARNING: We may need a custom relocation function
            // if we change the thing to not be contiguous.
        complain_overflow_bitfield,
        bfd_elf_generic_reloc,
        "R_FAVOR_IMM64_PCREL",
        false,
        0x00000000,
        0x0FFFF000,
        true),

    HOWTO(R_FAVOR_BIT32_LI_PCREL,
        31, // we want the sign bit
        4,  // 4 bytes
        32, // 32 bit relocation
        true,
        28, // Place in the lsb of the funct field
        complain_overflow_signed,
        bfd_elf_generic_reloc,
        "R_FAVOR_BIT32_LI_PCREL",
        false,
        0x00000000,
        0x10000000,
        true),

    HOWTO(R_FAVOR_BIT64_LI_PCREL,
        63, // we want the sign bit
        4,  // 4 bytes
        64, // 64 bit relocation
        true,
        28, // Place in the lsb of the funct field
        complain_overflow_signed,
        bfd_elf_generic_reloc,
        "R_FAVOR_BIT64_LI_PCREL",
        false,
        0x00000000,
        0x10000000,
        true),
};

#define FAVOR_RELOC_TABLE_SIZE (sizeof(favor_elf_howto_table) / sizeof(*favor_elf_howto_table))

static reloc_howto_type*
favor_elf_reloc_type_lookup(bfd *abfd ATTRIBUTE_UNUSED, bfd_reloc_code_real_type code) {
    if(code == BFD_RELOC_NONE)                 return &favor_elf_howto_table[R_FAVOR_NONE];
    if(code == BFD_RELOC_FAVOR_J22_PCREL)      return &favor_elf_howto_table[R_FAVOR_J22_PCREL];
    if(code == BFD_RELOC_FAVOR_IMM16_PCREL)    return &favor_elf_howto_table[R_FAVOR_IMM16_PCREL];
    if(code == BFD_RELOC_FAVOR_IMM32_PCREL)    return &favor_elf_howto_table[R_FAVOR_IMM32_PCREL];
    if(code == BFD_RELOC_FAVOR_IMM48_PCREL)    return &favor_elf_howto_table[R_FAVOR_IMM48_PCREL];
    if(code == BFD_RELOC_FAVOR_IMM64_PCREL)    return &favor_elf_howto_table[R_FAVOR_IMM64_PCREL];
    if(code == BFD_RELOC_FAVOR_BIT32_LI_PCREL) return &favor_elf_howto_table[R_FAVOR_BIT32_LI_PCREL];
    if(code == BFD_RELOC_FAVOR_BIT64_LI_PCREL) return &favor_elf_howto_table[R_FAVOR_BIT64_LI_PCREL];
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


static bfd_reloc_status_type
favor_final_link_relocate (reloc_howto_type *howto,
			   bfd *input_bfd,
			   asection *input_section,
			   bfd_byte *contents,
			   Elf_Internal_Rela *rel,
			   bfd_vma relocation)
{
    return _bfd_final_link_relocate(howto, input_bfd, input_section,
        contents, rel->r_offset, relocation, rel->r_addend);
}

static int
favor_elf_relocate_section (bfd *output_bfd,
			    struct bfd_link_info *info,
			    bfd *input_bfd,
			    asection *input_section,
			    bfd_byte *contents,
			    Elf_Internal_Rela *relocs,
			    Elf_Internal_Sym *local_syms,
			    asection **local_sections)
{
    Elf_Internal_Shdr *symtab_hdr;
    struct elf_link_hash_entry **sym_hashes;
    Elf_Internal_Rela *rel;
    Elf_Internal_Rela *relend;

    symtab_hdr = &elf_tdata(input_bfd)->symtab_hdr;
    sym_hashes = elf_sym_hashes(input_bfd);
    relend = relocs + input_section->reloc_count;

    for(rel = relocs; rel < relend; ++rel) {
        unsigned long r_symndx;
        int r_type;
        Elf_Internal_Sym *sym = NULL;
        asection *sec = NULL;
        bfd_vma relocation;
        bfd_reloc_status_type r;
        const char *name;
        struct elf_link_hash_entry *h = NULL;
        reloc_howto_type *howto;

        r_type   = ELF32_R_TYPE(rel->r_info);
        r_symndx = ELF32_R_SYM(rel->r_info);
        howto    = favor_elf_howto_table + r_type;

        if(r_symndx < symtab_hdr->sh_info) {
            sym = local_syms + r_symndx;
            sec = local_sections[r_symndx];
            relocation = _bfd_elf_rela_local_sym(output_bfd, sym, &sec, rel);
        
            name = bfd_elf_string_from_elf_section(input_bfd, symtab_hdr->sh_link, sym->st_name);
            name = name == NULL ? bfd_section_name(sec) : name;
        }
        else {
            bool unresolved_reloc, warned, ignored;

            RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
                r_symndx, symtab_hdr, sym_hashes,
                h, sec, relocation,
                unresolved_reloc, warned, ignored);

            name = h->root.root.string;
        }

        if(sec != NULL && discarded_section(sec))
            RELOC_AGAINST_DISCARDED_SECTION(info, input_bfd, input_section, rel, 1, relend, howto, 0, contents);

        if(bfd_link_relocatable(info)) continue;

        r = favor_final_link_relocate(howto, input_bfd, input_section, contents, rel, relocation);
        if(r != bfd_reloc_ok) {
            /* TODO: Error messages? Copy from moxie as always? */
            (*info->callbacks->warning)(info, "relocation error", name, input_bfd, input_section, rel->r_offset);
        }
    }

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

#define elf_backend_relocate_section    favor_elf_relocate_section

#include "elf32-target.h"