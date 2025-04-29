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
    RELAX_LD,
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
    FAVOR_OP_TABLE_INSTALL(int3)
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

    uint32_t type;
};

static char*
parse_ty(char *str, struct ty *out) {
    char *num_start, *num_end;
    if(*str != '.') { as_bad("Expected type specifier."); return str; }
    str++;

    switch(*str) { \
        case 'u': out->u = 1; out->f = 0; out->type = TYPE_U; break;
        case 's': out->u = 0; out->f = 0; out->type = TYPE_S; break;
        case 'f': out->u = 0; out->f = 1; out->type = TYPE_F; break;
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
    char *op_beg, *op_end;
    struct insn insn = {0};
    struct favor_op_info *op_info;
    char was;
    uint32_t src1, src2, dst;
    uint32_t replication;
    struct ty ty;
    expressionS exp;
    char *where = NULL;

    // printf("line = [%s]\n", str);
    
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

        switch(op_info->parser) {
            case PARSE_SINGLETON: {
                PARSE_CONDITIONAL();
                insn = mk_singleton(conditional, op_info->funct);
                break;
            }
            case PARSE_3ARG: {
                PARSE_TY();
                PARSE_CONDITIONAL();

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
            case PARSE_JUMP: {
                PARSE_CONDITIONAL();
                insn.p_opcode = OP_JUMP;
                insn.jump.and_link = 0;
                insn.jump.funct = op_info->funct;
                insn.jump.immediate = 0; // fixup

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
                PARSE_TY();
                PARSE_CONDITIONAL();
                //if(!ty.f && !ty.u) { as_bad("Use an unsigned load instead"); return; }

                input_line_pointer = str;
                if(!parse_reg_into(&str, &dst, false, ty.f)) {
                    as_bad("Expected destination register"); return;
                }

                if(*str != ',') {
                    as_bad("Expected comma after destination."); return;
                }
                str++;

                printf("got reg: %u\n", dst);

                // Now there are a few options.
                // 1. We have a `[reg1 + reg2 + const]` expression.
                // 2. We have a `[symbol]` expression.
                // 3. We have a `constant number` expression.
                // 4. We have a `symbol` expression (treated as a constant).
                if(*str == '[') {
                    as_bad("TODO: Loads from registers and such.");
                    return;
                }

                // Okay, we're in the 3rd/4th case. use the expression()
                // functionality.
                //
                // We really want to figure out how to do this in a simpler way...
                // but for now, just frag_more and then create a crazy fixup.
                input_line_pointer = str;
                expression(&exp);
                // if(exp.X_add_symbol) {
                //     printf("sym: %d\n", symbol_resolved_p(exp.X_add_symbol));
                // }
                // else {
                //     printf("num: %ld\n", exp.X_add_number);
                // }

                // This will be replaced by md_convert_frag. We need to provide
                // it with the correct starting info though. Ferry the size through
                // the imm field.
                insn = mk_ld_imm(conditional, dst, ty.sz, ty.f, ty.u ? LDI0U : LDI0S);
                output(where, favor_encode(insn));
                //s
                //insn = mk_ld_imm(conditional, dst, 0, ty.f, ty.vec, LS_IMM_LD64);
                end_frag_with_exp(&exp,
                    12,
                    0,
                    RELAX_LD);

               

                return;
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
    bool is_signed;
    uint64_t value;

    struct insn *insn;

    fragS *fragp;
    expressionS exp;
    char *where;
};

static void
emit_li_info(struct li_info *li, uint64_t shift) {
    enum bfd_reloc_code_real reloc = BFD_RELOC_FAVOR_IMM16_PCREL;
    
    switch(shift) {
        case 0:  reloc = BFD_RELOC_FAVOR_IMM16_PCREL; break;
        case 16: reloc = BFD_RELOC_FAVOR_IMM32_PCREL; break;
        case 32: reloc = BFD_RELOC_FAVOR_IMM48_PCREL; break;
        case 48: reloc = BFD_RELOC_FAVOR_IMM64_PCREL; break;
    }

    if(li->is_relocation) {
        if(!li->is_pcrel) {
            as_bad_where(li->fragp->fr_file, li->fragp->fr_line, "Non-pcrel relocations are not supported yet.");
        }
        fix_new_exp (li->fragp,
            (li->where - li->fragp->fr_literal),
            4,
            &li->exp,
            true,
            reloc);
    }
    else {
        // If we aren't relocating the value, we just emit it directly.
        li->insn->ld_imm.imm = li->value >> shift;
    }
    output(li->where, favor_encode(*li->insn));

    li->where += 4;
}

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT asec ATTRIBUTE_UNUSED,
		 fragS *fragp)
{
    struct insn insn;
    struct li_info li = {0};
    uint32_t size = 0;

    // Initial instruction.
    insn = favor_decode(read_code(fragp->fr_literal + fragp->fr_fix - 4));
    li.fragp = fragp;
    li.insn = &insn;
    li.where = fragp->fr_literal + fragp->fr_fix - 4;

    li.value = (uint64_t)((int64_t)fragp->fr_offset);

    if(fragp->fr_symbol) {
        li.value += S_GET_VALUE(fragp->fr_symbol);
        //relocation = !S_IS_DEFINED(fragp->fr_symbol);
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
    printf("convert frag: sym value = %lu\n", li.value);
    
    li.exp.X_add_number = fragp->fr_offset;
    li.exp.X_add_symbol = fragp->fr_symbol;
    li.exp.X_op = (li.exp.X_add_symbol ? O_symbol : O_constant);

    //fragp->fr_fix += 12;
    

    // The insn was created by the md_assemble function. At this point, we just
    // rewrite the funct field.
    gas_assert(insn.p_opcode == POP_LD_IMM);
    size = insn.ld_imm.imm;
    li.is_signed = insn.ld_imm.funct == LDI0S;

    if(li.is_pcrel) {
        if(!li.is_signed) {
            as_bad_where(fragp->fr_file, fragp->fr_line, "Expected signed load for pc-rel load.");
        }
        if(size < 2) {
            as_bad_where(fragp->fr_file, fragp->fr_line, "Expected 32 or 64 bit load for pc-rel load.");
        }
    }
    
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
            as_bad_where(fragp->fr_file, fragp->fr_line, "Unknown size for load.");
        }
    }

sz_64:
    emit_li_info(&li, 48);

    insn.ld_imm.funct = LDI2O;
    emit_li_info(&li, 32);

    insn.ld_imm.funct = LDI1O;
sz_32:
    emit_li_info(&li, 16);

    if(li.is_pcrel) {
        insn.ld_imm.funct = LDI0OPC;
    }
    else {
        insn.ld_imm.funct = LDI0O;
    }
sz_16:
    emit_li_info(&li, 0);

    valueT old = fragp->fr_fix;
    fragp->fr_fix = (uintptr_t)li.where - (uintptr_t)fragp->fr_literal;
    printf("grew by: %lu octets", (fragp->fr_fix - old));

    if(fragp->fr_next) {
        fragp->fr_next->fr_address = fragp->fr_address + fragp->fr_fix;
    }
}

int
md_estimate_size_before_relax (fragS* fragp, segT) {
    return fragp->fr_fix + 16; /* Worst case...? */
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
        insn |= ((uint32_t)val >> 2) << 6;
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
        printf("got relocation from buf: %p -> %x | %x\n", buf, insn, *(uint32_t*)(buf));
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