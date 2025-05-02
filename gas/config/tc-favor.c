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

enum relax_types {
    RELAX_LD = 1,
};

void
md_operand(expressionS *exp ATTRIBUTE_UNUSED) {}

static void
install_op_info(struct favor_op_info *info) {
    /* Use a NULL name field to indicate a reserved slot in the table. */
    if(!info->name) {
        return;
    }

    /* First, place the new info into the hash, but don't replace old info. */
    void **slot = str_hash_insert(opcode_hash, info->name, info, 0);
    if(slot) {
        string_tuple_t *elt = *slot;
        /* If there was an old slot, thread our new item into the linked list.
         * Note: We have to cast away the const but that should be fine. */
        info->next = (void*)elt->value;
        elt->value = info;
    }
}

/**
 * Initialization.
 */
void
md_begin(void) {
    size_t i;
    opcode_hash = str_htab_create();

#define FAVOR_OP_TABLE_INSTALL(table) \
for(i = 0; i < favor_op_ ## table ## _count; ++i) { \
    struct favor_op_info *op = &favor_op_ ## table[i]; \
    install_op_info(op); \
}

    FAVOR_OP_TABLE_INSTALL(singleton)
    FAVOR_OP_TABLE_INSTALL(jump)
    FAVOR_OP_TABLE_INSTALL(int3)
    FAVOR_OP_TABLE_INSTALL(int2)
    FAVOR_OP_TABLE_INSTALL(ld_imm)

    FAVOR_OP_TABLE_INSTALL(psuedo)
#undef FAVOR_OP_TABLE_INSTALL

    reg_hash = str_htab_create();
    for(i = 0; i < favor_reg_table_size; ++i) {
        struct favor_reg_info *reg = &favor_reg_table[i];
        str_hash_insert(reg_hash, reg->name, reg, 0);
    }

    bfd_set_arch_mach(stdoutput, TARGET_ARCH, 0);

    literal_prefix_dollar_hex = false;
}

static void
output(void *where, uint32_t code) {
    unsigned char *output = where;
    output[0] = (code >> 0)  & 0xFF;
    output[1] = (code >> 8)  & 0xFF;
    output[2] = (code >> 16) & 0xFF;
    output[3] = (code >> 24) & 0xFF;
}

static uint32_t
read_code(void *where) {
    unsigned char *input = where;
    uint32_t code = 0;

    code |= (input[0] << 0) ;
    code |= (input[1] << 8) ;
    code |= (input[2] << 16);
    code |= (input[3] << 24);

    return code;
}

static char*
skip_whitespace(char *str) {
    while(is_whitespace(*str)) ++str;
    return str;
}

static char*
skip_opcode(char *str) {
    while(!is_whitespace(*str) && (*str != '.') && (*str != '?') && (*str != '~') && (*str != ',') && !is_end_of_line(*str)) ++str;
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

    uint32_t type;
};

struct ty_spec {
    bool u;
    bool s;
    bool f;
    bool r;
};

static struct ty_spec spec_num = {
    .u = true,
    .s = true,
    .f = true,
    .r = false
};

static struct ty_spec spec_li = {
    .u = true,
    .s = false,
    .f = true,
    .r = true,
};

static char*
parse_ty(char *str, struct ty *out, struct ty_spec *spec) {
    char *num_start, *num_end;
    if(*str != '.') { as_bad("Expected type specifier."); return str; }
    str++;

#define CHECK(x) do {\
    if(!spec->x) { as_bad("Operation does not support type specifier %c.", *str); return str; } \
} while(0)
    switch(*str) {
        case 'u': CHECK(u); out->u = 1; out->f = 0; out->type = TYPE_U; break;
        case 's': CHECK(s); out->u = 0; out->f = 0; out->type = TYPE_S; break;
        case 'f': CHECK(f); out->u = 0; out->f = 1; out->type = TYPE_F; break;
        case 'r': CHECK(r); out->u = 1; out->f = 0; out->type = TYPE_R; break;
        default: as_bad("Unknown type specifier %c.", *str); return str;
    }
#undef CHECK

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

    if(num_start[0] == '8' && num_start + 1 == num_end) {
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
        char *vec_start, *vec_end;
        str++;
        vec_start = str;
        vec_end = (str = skip_number(str));
        if((size_t)(vec_end - vec_start) > 1) {
            as_bad("Unknown vector size.");
            return str;
        }
        if(*vec_start < '1' || *vec_start > '4') {
            as_bad("Unknown vector size.");
            return str;
        }
        out->vec = *vec_start - '1';
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

static void
parse_replication(char **str, uint32_t *out) {
    *out = 0;
    for(int i = 0; i < 3; ++i) {
        if(**str == '.') { (*str)++; }
        else {
            if(i > 0) {
                as_bad("Expected exactly 0 or 3 trailing periods.");
            }
            return;
        }
    }
    *out = 1;
}

/*
static bool
try_parse_reg_into(char **str, uint32_t *out, bool comma_first, uint32_t f) {
    struct favor_reg_info reg_info;
    bool success;

    *str = parse_reg(*str, &reg_info, comma_first, &success);
    if(!success) return false;

    if( f && !reg_info.f) { return false; }
    if(!f &&  reg_info.f) { return false; }

    *out = reg_info.reg_mask;
    return true;
}
*/

static void
end_frag_with_exp(expressionS *exp, size_t max_chars, size_t var, relax_substateT substate) {
    symbolS *sym;
    offsetT offset;
    switch (exp->X_op)
    {
    case O_symbol:
        sym = exp->X_add_symbol;
        offset = exp->X_add_number;
    break;
    case O_constant:
        sym = NULL;
        offset = exp->X_add_number;
    break;
    case O_big: {
        sym = NULL;
        uint64_t value = 0;
        // TODO: Specify the size of this, and convert to either 32-bit or
        // 64-bit float depending.
        LITTLENUM_TYPE words[4];
        gen_to_words(words, 4, 11);
        value |= (uint64_t)(words[3] & 0xFF) << 0ULL;
        value |= (uint64_t)(words[3] >> 8)   << 8ULL;
        value |= (uint64_t)(words[2] & 0xFF) << 16ULL;
        value |= (uint64_t)(words[2] >> 8)   << 24ULL;
        value |= (uint64_t)(words[1] & 0xFF) << 32ULL;
        value |= (uint64_t)(words[1] >> 8)   << 40ULL;
        value |= (uint64_t)(words[0] & 0xFF) << 48ULL;
        value |= (uint64_t)(words[0] >> 8)   << 56ULL;
        offset = value;
        break;
    }
    default:
        sym = make_expr_symbol (exp);
        offset = 0;
    break;
    }
    frag_var (rs_machine_dependent, max_chars, var,
        substate, sym, offset, NULL/*offset, opcode*/);
}

static struct favor_op_info*
lookup_type(struct favor_op_info *chain, uint32_t type) {
    struct favor_op_info *head = chain;
    while(chain) {
        if(chain->type == type) {
            return chain;
        }
        chain = chain->next;
    }

    switch(type) {
        case TYPE_U: as_bad("No such unsigned operation '%s'.", head->name); break;
        case TYPE_S: as_bad("No such signed operation '%s'.", head->name); break;
        case TYPE_F: as_bad("No such floating-point operation '%s'.", head->name); break;
    }

    return NULL;
}

void
md_assemble(char *str) {
    uint32_t conditional = 0;
    uint32_t is_return = 0;
    char *op_beg, *op_end;
    struct insn insn = {0};
    struct favor_op_info *op_info;
    char was;
    uint32_t src1, src2, dst;
    uint32_t replication;
    struct ty ty;
    expressionS exp;
    char *where = NULL;
    
    /* For now, if we find 'a' on the string, output 4 bytes.. */

    /* We must allocate the frag_more before reparse_insn due to how ret works.
     * Note that this means all instructions should be aware that 4 bytes have
     * already been allocated (which the li logic is right now). */
    where = frag_more (4);

reparse_insn:
    op_beg = (str = skip_whitespace(str));
    op_end = (str = skip_opcode(str));

#define PARSE_CONDITIONAL(enable_negated) do { \
    if(*str == '?') { conditional = 1; str++; } \
    if(enable_negated && (*str == '~')) { conditional = 2; str++; } \
} while(0)

#define PARSE_TY(spec) do { \
    str = parse_ty(str, &ty, &spec); \
} while(0)

#define TY_FUNCT() (ty.f ? op_info->funct_f : (ty.u ? op_info->funct_u : op_info->funct_s))

    was = *op_end;
    *op_end = '\0';
    op_info = str_hash_find(opcode_hash, op_beg);
    *op_end = was;

    if(op_info) {
        

        switch(op_info->parser) {
            case PARSE_PSUEDO_RET: {
                if(*str == '.') {
                    str++;
                    is_return = 1;
                    goto reparse_insn;
                }
                else {
                    // Nop return.
                    is_return = 1;
                    op_info = &favor_op_singleton[1];
                    goto parse_singleton;
                }
                break;
            }
            case PARSE_SINGLETON: {
parse_singleton:
                PARSE_CONDITIONAL(false);
                insn = mk_singleton(conditional, op_info->funct, is_return);
                break;
            }
            case PARSE_3ARG: {
                PARSE_TY(spec_num);
                PARSE_CONDITIONAL(false);

                op_info = lookup_type(op_info, ty.type);
                if(!op_info) break;

                if(!parse_reg_into(&str, &dst , false, ty.f)) return;
                if(!parse_reg_into(&str, &src1, true , ty.f)) return;
                if(!parse_reg_into(&str, &src2, true , ty.f)) return;
                parse_replication(&str, &replication);

                insn = (ty.f ? mk_float3 : mk_int3)(replication, conditional,
                    dst, src1, src2, ty.sz, ty.vec, op_info->funct);
                break;
            }
            case PARSE_2ARG: {
                PARSE_TY(spec_num);
                PARSE_CONDITIONAL(false);

                op_info = lookup_type(op_info, ty.type);
                if(!op_info) break;

                if(!parse_reg_into(&str, &dst, false, ty.f)) return;
                if(!parse_reg_into(&str, &src2, true, ty.f)) return;
                parse_replication(&str, &replication);

                insn = (ty.f ? mk_float2 : mk_int2)(replication, conditional,
                    dst, src2, ty.sz, ty.vec, op_info->funct);

                break;
            }
            case PARSE_JUMP: {
                insn.p_opcode = OP_JUMP;
                insn.jump.and_link = 0;
                insn.jump.funct = op_info->funct;
                insn.jump.immediate = 0; // fixup
                
                if(*str == '.') {
                    str++;
                    if(*str == 'l') {
                        str++;
                        insn.jump.and_link = 1;
                    }
                    else {
                        as_bad("Unexpected postfix after jump instruction.");
                    }
                }

                PARSE_CONDITIONAL(true);

                if(conditional && op_info->funct > J_BL) {
                    as_bad("Jump type an only be represented as unconditional.");
                }
                if(conditional == 1) {
                    // Regular conditional -- these are 0-9, so subtract 20.
                    insn.jump.funct -= 20;
                }
                if(conditional == 2) {
                    // Inverted conditional -- these are 10-19, so subtract 10.
                    insn.jump.funct -= 10;
                }

                input_line_pointer = str;
                expression(&exp);

                fix_new_exp (frag_now,
                    (where - frag_now->fr_literal),
                    4,
                    &exp,
                    true,
                    BFD_RELOC_FAVOR_J22_PCREL);

                break;
            }
            case PARSE_PSUEDO_LI: {
                PARSE_TY(spec_li);
                PARSE_CONDITIONAL(false);

                input_line_pointer = str;
                if(!parse_reg_into(&str, &dst, false, ty.f)) {
                    as_bad("Expected destination register"); return;
                }

                if(*str != ',') {
                    as_bad("Expected comma after destination."); return;
                }
                str++;

                // Now there are two options. Both are handled by expression().
                // 1. We have a `constant number` expression.
                // 2. We have a `symbol` expression (treated as a constant).

                input_line_pointer = str;
                expression(&exp);

                // This will be replaced by md_relax_frag. We need to provide
                // it with the correct starting info though. Ferry the size through
                // the imm field, and the type through the funct field.
                insn = mk_ld_imm(conditional, dst, ty.sz, ty.f, ty.type);
                output(where, favor_encode(insn));

                end_frag_with_exp(&exp,
                    12,
                    0,
                    RELAX_LD);

                return;
            }
        }
        if(is_return) {
            if(insn.p_opcode != POP_SINGLETON && insn.p_opcode != OP_MISC) {
                as_bad("Instruction cannot be used as a return instruction.");
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

struct li_info {
    bool is_pcrel;
    bool is_relocation;
    bool has_written;
    bool do_output;
    uint64_t value;

    struct insn *insn;

    fragS *fragp;
    expressionS exp;
    char *where;

    int lhs_trunc_now;
    int lhs_trunc_prev;

    int bytes_written;
};

enum {
    LI_TRUNC_NONE,
    LI_TRUNC_ALL0,
    LI_TRUNC_ALL1,
};

static int
compute_li_truncation(struct li_info *li, uint64_t shift) {
    uint64_t value = (li->value >> shift) & 0xFFFF;
    if(value == 0) {
        return LI_TRUNC_ALL0;
    }
    if(value == 0xFFFF) {
        return LI_TRUNC_ALL1;
    }
    return LI_TRUNC_NONE;
}

static bool
may_truncate_li_lhs(struct li_info *li, uint64_t shift, bool is_msh) {
    if(li->is_relocation) return false;

    int truncation = compute_li_truncation(li, shift);
    li->lhs_trunc_prev = li->lhs_trunc_now;
    /* Always reset lhs_truncation to NONE unless we actually truncate. */
    li->lhs_trunc_now = LI_TRUNC_NONE;
    
    if(truncation == LI_TRUNC_ALL0) {
        if(shift == 0 && !li->has_written) {
            // We can't truncate an ALL0 if we haven't generated any instructions
            // yet.
            return false;
        }
        // IMPORTANT: Only update truncation when we return true. That way,
        // we can correctly detect strings of sign bits.

        // For ALL0, we want to extend an ALL0 truncation from the LHS, but
        // if we ended up with a 0 hole, that isn't extending the LHS truncation.
        // in that case, don't update the truncation.
        //
        // So only update it 1) if we are msh, or 2) if the previous was also ALL0.
        if(is_msh || li->lhs_trunc_prev == LI_TRUNC_ALL0) {
            li->lhs_trunc_now = truncation;
        }
        return true;
    }

    if(truncation == LI_TRUNC_ALL1) {
        // There's no way to truncate if it's in the lowest half.
        if(shift == 0) return false;
        // For the most significant halfword, we can always truncate.
        // Otherwise, we have to have truncated an 0xFFFF before so that we
        // can use the S instruction.
        if(li->lhs_trunc_prev != LI_TRUNC_ALL1 && !is_msh) return false;

        // We always can truncate now. Note that this is because the *s
        // instructions are now defined to always do all 1s, not the sign bit.
        li->lhs_trunc_now = truncation;
        return true;
    }

    // Otherwise, we cannot truncate.
    return false;
}

static void
emit_li_info(struct li_info *li, uint64_t shift, bool is_msb) {
    /* Track this if we're not actually outputting. */
    li->bytes_written += 4;

    if(!li->do_output) return;

    enum bfd_reloc_code_real reloc = BFD_RELOC_FAVOR_IMM16_PCREL;
    offsetT reloc_add = 0;
    
    switch(shift) {
        case 0:  reloc = BFD_RELOC_FAVOR_IMM16_PCREL; reloc_add = -0; break;
        case 16: reloc = BFD_RELOC_FAVOR_IMM32_PCREL; reloc_add = -4; break;
        case 32: reloc = BFD_RELOC_FAVOR_IMM48_PCREL; reloc_add = -8; break;
        case 48: reloc = BFD_RELOC_FAVOR_IMM64_PCREL; reloc_add = -12; break;
    }

    if(li->is_relocation) {
        if(!li->is_pcrel) {
            as_bad_where(li->fragp->fr_file, li->fragp->fr_line, "Non-pcrel relocations are not supported yet.");
        }

        /* Create a clone of the expression, but with a slightly modified offset.
         *
         * Essentially, this makes sure all the relocations occur at exactly
         * the same location. */
        expressionS dup = li->exp;
        dup.X_add_number += reloc_add;

        fix_new_exp (li->fragp,
            (li->where - li->fragp->fr_literal),
            4,
            &dup,
            true,
            reloc);

        /* For a 32-bit load, we also need a relocation that will change between
         * li1u and li2s as appropriate, which also needs the new expression. */
        if(shift == 16 && is_msb) {
            fix_new_exp(li->fragp,
                (li->where - li->fragp->fr_literal),
                4,
                &dup,
                true,
                BFD_RELOC_FAVOR_BIT32_LI_PCREL);
        }
    }
    else {
        // If we aren't relocating the value, we just emit it directly.
        li->insn->ld_imm.imm = li->value >> shift;
    }
    output(li->where, favor_encode(*li->insn));

    li->has_written = true;

    li->where += 4;
}

static uint32_t
li_compute_funct(struct li_info *li, bool is_msh, uint32_t u, uint32_t s, uint32_t o) {
    // Always return U for the most significant bit.
    if(is_msh) return u;

    // If we are truncating the LHS to be ALL1 or ALL0, then we need to use
    // the appropriate sign-extending instruction.
    if(!li->is_relocation) {
        if(li->lhs_trunc_prev == LI_TRUNC_ALL1) {
            return s;
        }
        if(li->lhs_trunc_prev == LI_TRUNC_ALL0) {
            return u;
        }
    }

    return o;
}

static int
do_convert_frag(fragS *fragp, bool do_output) {
    struct insn insn;
    struct li_info li = {0};
    uint32_t size = 0;

    // Initial instruction.
    insn = favor_decode(read_code(fragp->fr_literal + fragp->fr_fix - 4));
    li.fragp = fragp;
    li.insn = &insn;
    li.where = fragp->fr_literal + fragp->fr_fix - 4;

    li.value = (uint64_t)((int64_t)fragp->fr_offset);

    li.lhs_trunc_now = LI_TRUNC_NONE;
    li.lhs_trunc_prev = LI_TRUNC_NONE;
    li.has_written = false;
    li.do_output = do_output;

    if(fragp->fr_symbol) {
        li.value += S_GET_VALUE(fragp->fr_symbol);
        li.is_relocation = !S_IS_DEFINED(fragp->fr_symbol);
        // TODO: Kill parts of the value if we don't need the relocation.
        li.is_pcrel = S_GET_SEGMENT(fragp->fr_symbol)->output_section != bfd_abs_section_ptr;

        // Always relocate pcrel values in case they change at link time.
        if(li.is_pcrel) {
            li.is_relocation = true;
        }
    }
    else {
        li.is_pcrel = false;
        li.is_relocation = false;
    }
    
    li.exp.X_add_number = fragp->fr_offset;
    li.exp.X_add_symbol = fragp->fr_symbol;
    li.exp.X_op = (li.exp.X_add_symbol ? O_symbol : O_constant);

    //fragp->fr_fix += 12;
    

    // The insn was created by the md_assemble function. At this point, we just
    // rewrite the funct field.
    gas_assert(insn.p_opcode == POP_LD_IMM);
    size = insn.ld_imm.imm;
    // Reset the immediate back to 0 after reading the size.
    insn.ld_imm.imm = 0;
    int type = insn.ld_imm.funct;
    insn.ld_imm.funct = 0;

    if(li.is_pcrel) {
        if(type != TYPE_R) {
            if(do_output) as_bad_where(fragp->fr_file, fragp->fr_line, "Expected relative load for pc-relative operand.");
        }
        if(size < 2) {
            if(do_output) as_bad_where(fragp->fr_file, fragp->fr_line, "Expected 32 or 64 bit load for pc-rel load.");
        }
    }
    else {
        if(type != TYPE_U && type != TYPE_F) {
            if(do_output) as_bad_where(fragp->fr_file, fragp->fr_line, "Expected unsigned or floating-point load for absolute operand.");
        }
    }
    
    /**
     * Carry the is_msh bool into the goto switch statement so that when we
     * reach the relevant part of the code, we can compute it.
     */
    bool is_msh = true;

    /**
     * It is important to be careful about signed/unsigned values.
     * - The only use of the ldi*s instructions is to sign-extend a *smaller*
     *   value into a larger one. So, for example, we can sign-extend a 32-bit
     *   immediate into a 64-bit signed value. But importantly, we do NOT call
     *   ldi1s if loading a 32-bit value! Because we want the upper 64 bits to
     *   be 0 in that case.
     * - The following sign extensions are available:
     *   - Anything -> 64 bits using the s variants
     *   - 1 half-word -> 32 bits using LDI032S
     *   - There is no need to sign-extend to 16 or 8 bits as the 16 bit 
     *     immediate is enough to represent all values.
     */
    switch(size) {
        case 3: insn.ld_imm.funct = LDI3U; goto sz_64;
        case 2: {
            insn.ld_imm.funct = LDI1U;
            goto sz_32;
        }
        case 1: insn.ld_imm.funct = LDI0U; goto sz_16;
        case 0: {
            // TODO: Check that values fit?
            li.value &= 0xFF;
            insn.ld_imm.funct = LDI0U;
            goto sz_16;
        }
        default: {
            if(do_output) as_bad_where(fragp->fr_file, fragp->fr_line, "Unknown size for load.");
        }
    }

    

sz_64:
    if(!may_truncate_li_lhs(&li, 48, is_msh)) {
        emit_li_info(&li, 48, is_msh);
    }
    is_msh = false;

    if(!may_truncate_li_lhs(&li, 32, is_msh)) {
        insn.ld_imm.funct = li_compute_funct(&li, is_msh, LDI2U, LDI2S, LDI2O);
        emit_li_info(&li, 32, is_msh);
    }

sz_32:
    if(!may_truncate_li_lhs(&li, 16, is_msh)) {
        insn.ld_imm.funct = li_compute_funct(&li, is_msh, LDI1U, LDI1S, LDI1O);
        emit_li_info(&li, 16, is_msh);
    }
    is_msh = false;

    
sz_16:
    if(!may_truncate_li_lhs(&li, 0, is_msh)) {
        if(li.is_pcrel) {
            // TODO: For pcrel instructions, we need to always emit
            // li1s instead of li1u.
            insn.ld_imm.funct = LDI0OPC;
        }
        else {
            insn.ld_imm.funct = li_compute_funct(&li, is_msh, LDI0U, LDI0S, LDI0O);
            // Special case: If we are LDIOS, but our actual size is 32 bits, then
            // we actually want 32S.
            if(insn.ld_imm.funct == LDI0S && size == 2) {
                insn.ld_imm.funct = LDI0S32;
            }
        }
        emit_li_info(&li, 0, is_msh);
    }



    //valueT old = fragp->fr_fix;
    if(do_output) {
        fragp->fr_fix = (uintptr_t)li.where - (uintptr_t)fragp->fr_literal;
    }
    return li.bytes_written - 4;
}

/**
 * This method appears to be called in a loop to relax every frag and move things
 * around as appropriate.
 * 
 * As such, it is where we want to figure out the length for our li sequences.
 */
int
favor_relax_frag(segT asec ATTRIBUTE_UNUSED, fragS *fragp, int stretch ATTRIBUTE_UNUSED) {
    if(fragp->fr_subtype == RELAX_LD) {
        fragp->fr_subtype = 0;
        return do_convert_frag(fragp, true);
    }
    else {
        return 0;
    }
}

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT asec ATTRIBUTE_UNUSED,
		 fragS *fragp ATTRIBUTE_UNUSED)
{
    /* This doesn't do anything anymore. Everything has been moved to relax. */
}

/*
 * My latest understanding of this is that it does not need to return an exact size.
 * The way that do_convert_frag is written, we can return an exact size if needed.
 */
int
md_estimate_size_before_relax (fragS* fragp, segT) {
    if(fragp->fr_subtype != RELAX_LD) return 0;

    /* RELAX_LD can at most be 12 more bytes. */
    return 12;
}

static uint32_t
get(char *buf) {
    unsigned char *buf2 = (unsigned char*)buf;
    uint32_t result = 0;
    result |= (uint32_t)buf2[0];
    result |= ((uint32_t)buf2[1] << 8);
    result |= ((uint32_t)buf2[2] << 16);
    result |= ((uint32_t)buf2[3] << 24);
    return result;
}

void
md_apply_fix(fixS *fixP ATTRIBUTE_UNUSED, valueT *valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED) {
    char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;
    uint64_t val = (uint64_t)*valP;
    // TODO: Check size fits?

    switch (fixP->fx_r_type)
    {
    // TODO: We probably don't want to use any of the base BFD types, and instead add our own
    // to bfd/bfd.h.
    case BFD_RELOC_FAVOR_J22_PCREL: {
        // TODO: CUstom relocation
        uint32_t insn = get(buf);
        insn |= (((uint32_t)val >> 2) & 0x3FFFFF) << 6;
        output(buf, insn);
        if(fixP->fx_addsy == NULL) {
            // Done with fixes that have no symbol, as they're always
            // PC-relative..?
            fixP->fx_done = 1;
        }
        //buf += 4;
        break;
    }
    case BFD_RELOC_FAVOR_IMM16_PCREL:
    case BFD_RELOC_FAVOR_IMM32_PCREL:
    case BFD_RELOC_FAVOR_IMM48_PCREL:
    case BFD_RELOC_FAVOR_IMM64_PCREL:
    {
        uint64_t shift = 0;
        if(fixP->fx_r_type == BFD_RELOC_FAVOR_IMM32_PCREL) shift = 16;
        if(fixP->fx_r_type == BFD_RELOC_FAVOR_IMM48_PCREL) shift = 32;
        if(fixP->fx_r_type == BFD_RELOC_FAVOR_IMM64_PCREL) shift = 48;

        // TODO: CUstom relocation
        uint32_t insn = get(buf);
        insn |= (uint32_t)((val >> shift) & 0xFFFF) << 12;
        output(buf, insn);
        if(fixP->fx_addsy == NULL) {
            // Done with fixes that have no symbol, as they're always
            // PC-relative..?
            fixP->fx_done = 1;
        }
        //buf += 4;
        break;
    }
    case BFD_RELOC_FAVOR_BIT32_LI_PCREL: {
        uint32_t insn = get(buf);
        // TODO: Check if fits?
        uint32_t bit = (val >> 31) & 1;
        insn |= bit << 28;
        output(buf, insn);
        if(fixP->fx_addsy == NULL) {
            // Done with fixes that have no symbol, as they're always
            // PC-relative..?
            fixP->fx_done = 1;
        }
        break;
    }
    default:
        /* TODO: We probably want BFD_RELOC_32, BFD_RELOC_64. At the very least
         * _64 for pointers. */
        as_bad("Cannot represent relocation type %s\n", bfd_get_reloc_code_name(fixP->fx_r_type));
        break;
        //abort();
    }
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
    rel->addend = fixp->fx_offset;
    rel->howto = bfd_reloc_type_lookup(stdoutput, r_type);

    if(rel->howto == NULL) {
        as_bad_where(fixp->fx_file, fixp->fx_line,
            _("Cannot represent relocation type %s"),
            bfd_get_reloc_code_name(r_type));

        rel->howto = bfd_reloc_type_lookup(stdoutput, BFD_RELOC_FAVOR_J22_PCREL);
    }

    return rel;
}

// No psuedo-ops yet.
const pseudo_typeS md_pseudo_table[] = {
    { 0, 0, 0 }
};