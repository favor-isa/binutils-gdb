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
static htab_t reg_hash;

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

    reg_hash = str_htab_create();
    for(i = 0; i < favor_reg_table_size; ++i) {
        struct favor_reg_info *reg = &favor_reg_table[i];
        str_hash_insert(reg_hash, reg->name, reg, 0);
    }

    bfd_set_arch_mach(stdoutput, TARGET_ARCH, 0);
}

static void
output(void *where, uint32_t code) {
    unsigned char *output = where;
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
    while(!is_whitespace(*str) && (*str != '.') && (*str != '?') && (*str != ',') && !is_end_of_line(*str)) ++str;
    return str;
}

static char*
skip_number(char *str) {
    while((*str >= '0') && (*str <= '9') && !is_end_of_line(*str)) ++str;
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

struct ty {
    uint32_t u : 1;
    uint32_t f : 1;
    uint32_t sz: 2;
    uint32_t vec: 2;
};

static char*
parse_ty(char *str, struct ty *out) {
    char *num_start, *num_end;
    if(*str != '.') { as_bad("Expected type specifier."); return str; }
    str++;

    switch(*str) { \
        case 'u': out->u = 1; out->f = 0; break;
        case 's': out->u = 0; out->f = 0; break;
        case 'f': out->u = 0; out->f = 1; break;
        default: as_bad("Unknown type specifier %c.", *str); return str;
    }

    str++;

    num_start = str;
    num_end = (str = skip_number(str));

    if(num_start == num_end) {
        as_bad("Expected type size.");
        return str;
    }

    if((size_t)(num_end - num_start) > 2) {
        as_bad("Unknown type size.");
        return str;
    }

    if(num_start[0] == '8' && num_start[1] == '\0') {
        out->sz = 0;
    }
    else if(num_start[0] == '1' && num_start[1] == '6') {
        out->sz = 1;
    }
    else if(num_start[0] == '3' && num_start[1] == '2') {
        out->sz = 2;
    }
    else if(num_start[0] == '6' && num_start[1] == '4') {
        out->sz = 3;
    }
    else {
        as_bad("Unknown type size.");
        return str;
    }

    if(*str == 'x') {
        //char *vec_start, *vec_end;
        //str++;
        // TODO VEC
    }
    else {
        out->vec = 0;
    }

    return str;
}

static char*
parse_reg(char *str, struct favor_reg_info *out, bool comma_first, bool *success) {
    char *reg_beg, *reg_end;
    char was;
    struct favor_reg_info *info;

    *success = false;

    str = skip_whitespace(str);
    if(comma_first) {
        if(*str != ',') { as_bad("Expected comma"); return str; }
        str++;
        str = skip_whitespace(str);
    }

    reg_beg = str;
    reg_end = (str = skip_opcode(str));

    was = *reg_end;
    *reg_end = '\0';
    info = str_hash_find(reg_hash, reg_beg);
    if(!info) {
        as_bad("Unknown register %s", reg_beg);
        *reg_end = was;
        return str;
    }
    *reg_end = was;

    memcpy(out, info, sizeof(*out));
    *success = true;
    return str;
}

static bool
parse_reg_into(char **str, uint32_t *out, bool comma_first, uint32_t f) {
    struct favor_reg_info reg_info;
    bool success;

    *str = parse_reg(*str, &reg_info, comma_first, &success);
    if(!success) return false;

    if( f && !reg_info.f) { as_bad("Expected floating-point register."); return false; }
    if(!f &&  reg_info.f) { as_bad("Expected integer register."); return false; }

    *out = reg_info.reg_mask;
    return true;
}

void
md_assemble(char *str) {
    uint32_t conditional = 0;
    char *op_beg, *op_end;
    struct insn insn = {0};
    struct favor_op_info *op_info;
    char was;
    uint32_t src1, src2, dst;
    struct ty ty;
    expressionS exp;
    char *where = NULL;

    printf("line = [%s]\n", str);
    
    /* For now, if we find 'a' on the string, output 4 bytes.. */

    op_beg = (str = skip_whitespace(str));
    op_end = (str = skip_opcode(str));

#define PARSE_CONDITIONAL() do { \
    if(*str == '?') { conditional = 1; str++; } \
} while(0)

#define PARSE_TY() do { \
    str = parse_ty(str, &ty); \
} while(0)

#define TY_FUNCT() (ty.f ? op_info->funct_f : (ty.u ? op_info->funct_u : op_info->funct_s))

    was = *op_end;
    *op_end = '\0';
    op_info = str_hash_find(opcode_hash, op_beg);
    *op_end = was;

    if(op_info) {
        where = frag_more (4);

        switch(op_info->opcode) {
            case OP_CC_MISC_SINGLETON: {
                PARSE_CONDITIONAL();
                insn = mk_basic_cc_misc(conditional, op_info->funct_cc);
                break;
            }
            case OP_CC_MISC: {
                // TODO
                break;
            }
            case OP_INT_FLOAT_3: {
                PARSE_TY();
                PARSE_CONDITIONAL();
                if(ty.f) {
                    if(op_info->funct_f < 0) { as_bad("No such floating-point operation."); return; }
                }
                else if(ty.u) {
                    if(op_info->funct_u < 0) { as_bad("No such unsigned integer operation."); return; }
                }
                else {
                    if(op_info->funct_s < 0) { as_bad("No such signed integer operation."); return; }
                }
                if(!parse_reg_into(&str, &dst , false, ty.f)) return;
                if(!parse_reg_into(&str, &src1, true , ty.f)) return;
                if(!parse_reg_into(&str, &src2, true , ty.f)) return;
                insn = (ty.f ? mk_float3 : mk_int3)(conditional, dst, src1, src2, ty.sz, ty.vec, TY_FUNCT());
                break;
            }
            case OP_JUMP: {
                PARSE_CONDITIONAL();
                insn.opcode = OP_JUMP;
                insn.jump.and_link = 0;
                insn.jump.funct = op_info->funct_j;
                insn.jump.immediate = 0; // fixup

                input_line_pointer = str;
                expression(&exp);

                fix_new_exp (frag_now,
                    (where - frag_now->fr_literal),
                    4,
                    &exp,
                    true,
                    BFD_RELOC_FAVOR_J23_PCREL);

                break;
            }
        }
        output(where, favor_encode(insn));
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

static uint32_t
get(char *buf) {
    uint32_t result = 0;
    result |= (uint32_t)buf[0];
    result |= ((uint32_t)buf[1] << 8);
    result |= ((uint32_t)buf[2] << 16);
    result |= ((uint32_t)buf[3] << 24);
    return result;
}

void
md_apply_fix(fixS *fixP ATTRIBUTE_UNUSED, valueT *valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED) {
    char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;
    uint32_t val = (uint32_t)*valP;
    // TODO: Check size fits?

    switch (fixP->fx_r_type)
    {
    // TODO: We probably don't want to use any of the base BFD types, and instead add our own
    // to bfd/bfd.h.
    case BFD_RELOC_FAVOR_J23_PCREL:
        // TODO: CUstom relocation
        printf("incoming reloc: %x\n", val);
        uint32_t insn = get(buf);
        printf("incoming insn: %x\n", insn);
        insn |= (val >> 2) << 9;
        printf("outgoing insn: %x\n", insn);
        output(buf, insn);
        //buf += 4;
        break;
    default:
        abort();
    }
}

void
md_number_to_chars(char *ptr, valueT use, int nbytes) {
    /* Everything on FAVOR is little endian. */
    number_to_chars_littleendian(ptr, use, nbytes);
}

arelent*
tc_gen_reloc(asection *section ATTRIBUTE_UNUSED, fixS *fixp) {
    printf("gen reloc? %s \n", S_GET_NAME(fixp->fx_addsy));
    arelent *rel;
    bfd_reloc_code_real_type r_type;

    rel = notes_alloc(sizeof(arelent));
    rel->sym_ptr_ptr = notes_alloc(sizeof(asymbol*));
    *rel->sym_ptr_ptr = symbol_get_bfdsym(fixp->fx_addsy);
    rel->address = fixp->fx_frag->fr_address + fixp->fx_where;

    r_type = fixp->fx_r_type;
    rel->addend = fixp->fx_offset;
    rel->howto = bfd_reloc_type_lookup(stdoutput, r_type);

    if(rel->howto == NULL) {
        as_bad_where(fixp->fx_file, fixp->fx_line,
            _("Cannot represent relocation type %s"),
            bfd_get_reloc_code_name(r_type));

        rel->howto = bfd_reloc_type_lookup(stdoutput, BFD_RELOC_FAVOR_J23_PCREL);
    }

    return rel;
}

// No psuedo-ops yet.
const pseudo_typeS md_pseudo_table[] = {
    { 0, 0, 0 }
};