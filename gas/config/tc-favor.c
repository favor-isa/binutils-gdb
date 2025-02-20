#include "as.h"
#include "safe-ctype.h"
#include "opcode/favor.h"

// NOTE: These must be provided.
const char comment_chars[] = "#";
const char line_separator_chars[] = ";";
const char line_comment_chars[] = "#";

// Steal these for now.
const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "fFdD";
// We will probably want to add fixed-point types through this..?

void
md_operand(expressionS *exp ATTRIBUTE_UNUSED) {}

/**
 * Initialization.
 */
void
md_begin(void) {
    bfd_set_arch_mach(stdoutput, TARGET_ARCH, 0);
}

static void
output(uint32_t code) {
    unsigned char *output = (void*)frag_more(4);
    output[0] = (code >> 0) & 0xFF;
    output[1] = (code >> 8) & 0xFF;
    output[2] = (code >> 16) & 0xFF;
    output[3] = (code >> 24) & 0xFF;
}

void
md_assemble(char *str) {
    /* For now, if we find 'a' on the string, output 4 bytes.. */

    while(*str == ' ') str++;

    if(*str == 'a' || *str == 'b' || *str == 'c') {
        output(0xF0F0F000 | (uint32_t)*str);
    }
}

const char*
md_atof(int type, char *litP, int *sizeP) {
    /* cribbed from random file.
     * Maybe we can do our fixed point numbers here? */
    return ieee_md_atof (type, litP, sizeP, true);
}

const char md_shortopts[] = "";
const struct option md_longopts[] = {
    {NULL, no_argument, NULL, 0}
};
const size_t md_longopts_size = sizeof(md_longopts);

/* no options yet. */
int
md_parse_option(int c ATTRIBUTE_UNUSED, const char *arg ATTRIBUTE_UNUSED) {
    return 0;
}

void
md_show_usage(FILE *stream ATTRIBUTE_UNUSED) { }

void
md_apply_fix(fixS *fixP ATTRIBUTE_UNUSED, valueT *valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED) {

}

void
md_number_to_chars(char *ptr, valueT use, int nbytes) {
    /* Everything on FAVOR is little endian. */
    number_to_chars_littleendian(ptr, use, nbytes);
}

arelent*
tc_gen_reloc(asection *section ATTRIBUTE_UNUSED, fixS *fixp) {
    arelent *rel;
    bfd_reloc_code_real_type r_type;

    rel = notes_alloc(sizeof(arelent));
    rel->sym_ptr_ptr = notes_alloc(sizeof(asymbol*));
    *rel->sym_ptr_ptr = symbol_get_bfdsym(fixp->fx_addsy);
    rel->address = fixp->fx_frag->fr_address + fixp->fx_where;

    r_type = fixp->fx_r_type;
    rel->addend = fixp->fx_addnumber;
    rel->howto = bfd_reloc_type_lookup(stdoutput, r_type);

    if(rel->howto == NULL) {
        as_bad_where(fixp->fx_file, fixp->fx_line,
            _("Cannot represent relocation type %s"),
            bfd_get_reloc_code_name(r_type));

        rel->howto = bfd_reloc_type_lookup(stdoutput, BFD_RELOC_32);
    }

    return rel;
}

// No psuedo-ops yet.
const pseudo_typeS md_pseudo_table[] = {
    { 0, 0, 0 }
};