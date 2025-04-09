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

static htab_t opcode_hash;

void
md_operand(expressionS *exp ATTRIBUTE_UNUSED) {}

/**
 * Initialization.
 */
void
md_begin(void) {
    size_t i;
    opcode_hash = str_htab_create();

    for(i = 0; i < favor_op_table_size; ++i) {
        struct favor_op_info *op = &favor_op_table[i];
        str_hash_insert(opcode_hash, op->name, op, 0);
    }

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

static char*
skip_whitespace(char *str) {
    while(is_whitespace(*str)) ++str;
    return str;
}

static char*
skip_opcode(char *str) {
    while(!is_whitespace(*str) && (*str != '.') && (*str != '?') && !is_end_of_line(*str)) ++str;
    return str;
}

// static bool
// match(char *s, char *e, const char *str) {
//     while(s != e) {
//         if(*s != *str) { return false; }
//         // *s == *str
//         if(*str == '\0') { return true; }
//         s++;
//         str++;
//     }

//     // now *str should be 0
//     return *str == '\0';
// }

// #define MATCH(s) match(op_beg, op_end, s)

void
md_assemble(char *str) {
    uint32_t conditional = 0;
    char *op_beg, *op_end;
    struct insn insn;
    struct favor_op_info *op_info;
    char was;
    /* For now, if we find 'a' on the string, output 4 bytes.. */

    op_beg = (str = skip_whitespace(str));
    op_end = (str = skip_opcode(str));

#define PARSE_CONDITIONAL() do { \
    if(*str == '?') { conditional = 1; str++; } \
} while(0)

    was = *op_end;
    *op_end = '\0';
    op_info = str_hash_find(opcode_hash, op_beg);
    *op_end = was;

    if(op_info) {
        switch(op_info->opcode) {
            case OP_CC_MISC_SINGLETON: {
                PARSE_CONDITIONAL();
                insn = mk_basic_cc_misc(conditional, op_info->funct_cc);
            }
        }
        output(favor_encode(insn));
    }
    else if(op_end != op_beg) {
        // Otherwise, invalid instruction.
        *op_end = '\0';
        as_bad("Invalid opcode '%s'.", op_beg);
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