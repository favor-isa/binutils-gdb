#include "sysdep.h"
#include <stdio.h>

#include "disassemble.h"

// TODO: Fgure out if we want these options.
#define STATIC_TABLE
#define DEFINE_TABLE

#include "opcode/favor.h"
#include "dis-asm.h"

/**
 * Struct used to track our disassembly progress for a single instruction.
 * Should just be stack-allocated.
 */
struct favor_dis_info {
    struct disassemble_info *info;

    /**
     * Number of characters printed as part of the "opcode" field. Used
     * to align everything to the same number of characters.
     */
    size_t oplen;
};

static
struct favor_op_info*
lookup_opcode(struct favor_op_info *table, size_t size, uint32_t funct) {
    if(funct < size) {
        if(table[funct].name == NULL) {
            /* Null name -- reserved slot, not real opcode. Return NULL in that
             * case. */
            return NULL;
        }
        return &table[funct];
    }
    return NULL;
}

#define LOOKUP_OPCODE(table, funct) lookup_opcode(favor_op_ ## table, favor_op_ ## table ## _count, funct)

#define FPRINTF(fmt, ...) info->info->fprintf_func(info->info->stream, fmt, ##__VA_ARGS__)

static void
pr_cond(struct favor_dis_info *info, uint32_t conditional) {
    if(conditional) {
        FPRINTF("?");
    }
    else {
        FPRINTF(" ");
    }
    for(size_t i = info->oplen; i < 8; ++i) {
        // Pad the string to 8 bytes?
        FPRINTF(" ");
    }
}

static void
pr_opname(struct favor_dis_info *info, const char *opname) {
    FPRINTF("%s", opname);
    info->oplen += strlen(opname);
}

static void
pr_type(struct favor_dis_info *info, uint32_t fp, uint32_t is_signed, uint32_t sz, uint32_t vec) {
#define PRINTF_CONST_OPLEN(conststr) do { FPRINTF(conststr); info->oplen += strlen(conststr); } while(0)

    PRINTF_CONST_OPLEN(".");

    if(fp) PRINTF_CONST_OPLEN("f");
    else if(is_signed) { PRINTF_CONST_OPLEN("s"); }
    else { PRINTF_CONST_OPLEN("u"); }

    switch(sz) {
        case 0: PRINTF_CONST_OPLEN("8"); break;
        case 1: PRINTF_CONST_OPLEN("16"); break;
        case 2: PRINTF_CONST_OPLEN("32"); break;
        case 3: PRINTF_CONST_OPLEN("64"); break;
    }
    switch(vec) {
        case 0: break;
        case 1: PRINTF_CONST_OPLEN("x2"); break;
        case 2: PRINTF_CONST_OPLEN("x3"); break;
        case 3: PRINTF_CONST_OPLEN("x4"); break;
    }
}

static void
pr_gpr(struct favor_dis_info *info, uint32_t reg, const char *after) {
    if(reg > favor_reg_table_size) return;

    FPRINTF("%s%s", favor_reg_table[reg].name, after);
}

static int32_t
sign_extend_32(uint32_t input, uint32_t bit) {
    if(input & (1 << bit)) {
        while(bit < (sizeof(input) * 8)) {
            input |= ((uint32_t)1 << bit);
            bit += 1;
        }
    }
    return (int32_t)input;
}

int
print_insn_favor(bfd_vma addr, struct disassemble_info *dis_info) {
    struct favor_dis_info the_info = {
        .info = dis_info,
        .oplen = 0
    };
    bfd_byte the_bytes[4];
    uint32_t opcode = 0;
    int err = dis_info->read_memory_func(addr, the_bytes, 4, dis_info);
    struct favor_dis_info * const info = &the_info;
    if(err) {
        dis_info->memory_error_func(err, addr, dis_info);
        return -1;
    }

    // Create opcode out of bytes.
    opcode |= the_bytes[0];
    opcode |= (the_bytes[1] << 8);
    opcode |= (the_bytes[2] << 16);
    opcode |= (the_bytes[3] << 24);

    // We read the opcode, disassemble it.
    struct insn insn = favor_decode(opcode);

    switch(insn.p_opcode) {
        case POP_SINGLETON: {
            struct favor_op_info *op = LOOKUP_OPCODE(singleton, insn.singleton.funct);
            if(!op) goto bad_op;
            pr_opname(info, op->name);
            pr_cond(info, insn.singleton.conditional);
            break;
        }
        case OP_MISC: {
            goto bad_op;
            break;
        }
        case POP_I3: {
            struct favor_op_info *op = LOOKUP_OPCODE(int3, insn.int3.funct);
            if(!info) goto bad_op;

            uint32_t is_signed = op->type == TYPE_S;
            
            pr_type(info, 0, is_signed, insn.int3.sz, insn.int3.vec);
            pr_cond(info, insn.int3.conditional);
            pr_gpr(info, insn.int3.dest, ", ");
            pr_gpr(info, insn.int3.src1, ", ");
            pr_gpr(info, insn.int3.src2, "");

            break;
        }
        case OP_JUMP: {
            int32_t jump_off = (sign_extend_32(insn.jump.immediate, 21) * 4);
            // We'll never hit the maximum negative value because the immediate
            // value isn't large enough.
            uint32_t jump_off_abs = (uint32_t)((jump_off < 0) ? -jump_off : jump_off);

            switch(insn.jump.funct) {
                case J_JUMP: pr_opname(info, "j"); break;
            }
            pr_cond(info, 0); // TODO conditional
            // TODO: Figure out sign extension? Also...

            FPRINTF("%c%#x ", " -"[jump_off < 0], jump_off_abs);
            FPRINTF("# 0x");
            dis_info->print_address_func(addr + jump_off, dis_info);
            break;
        }
        case POP_LD_IMM: {
            // Probably we want op to just be a psuedoop, so we keep it like this?
            struct favor_op_info *op = LOOKUP_OPCODE(ld_imm, insn.ld_imm.funct);
            if(!op) goto bad_op;

            pr_opname(info, op->name);
            pr_cond(info, insn.ld_imm.conditional);
            pr_gpr(info, insn.ld_imm.dest, ", ");
            FPRINTF("0x%x", insn.ld_imm.imm);
            break;
        }
        default:
bad_op:
            FPRINTF(".long 0x%08x", opcode);
            break;
    }

    return 4; // The number of bytes to advance. -1 on error?
}