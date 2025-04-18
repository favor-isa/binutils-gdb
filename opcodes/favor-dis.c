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

static const char*
lookup_opcode(struct favor_op_info *table, size_t size, uint32_t funct) {
    if(funct < size) {
        return table[funct].name;
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
    switch(reg) {
        case 0:  FPRINTF("a0%s", after); break;
        case 1:  FPRINTF("a1%s", after); break;
        case 2:  FPRINTF("a2%s", after); break;
        case 3:  FPRINTF("a3%s", after); break;
        case 4:  FPRINTF("a4%s", after); break;
        case 5:  FPRINTF("a5%s", after); break;
        case 6:  FPRINTF("a6%s", after); break;
        case 7:  FPRINTF("a7%s", after); break;
        case 8:  FPRINTF("v0%s", after); break;
        case 9:  FPRINTF("v1%s", after); break;
        case 10: FPRINTF("v2%s", after); break;
        case 11: FPRINTF("v3%s", after); break;
        case 12: FPRINTF("v4%s", after); break;
        case 13: FPRINTF("v5%s", after); break;
        case 14: FPRINTF("v6%s", after); break;
        case 15: FPRINTF("v7%s", after); break;
        case 16: FPRINTF("t0%s", after); break;
        case 17: FPRINTF("t1%s", after); break;
        case 18: FPRINTF("t2%s", after); break;
        case 19: FPRINTF("t3%s", after); break;
        case 20: FPRINTF("t4%s", after); break;
        case 21: FPRINTF("t5%s", after); break;
        case 22: FPRINTF("t6%s", after); break;
        case 23: FPRINTF("t7%s", after); break;
        case 24: FPRINTF("t8%s", after); break;
        case 25: FPRINTF("t9%s", after); break;
        case 26: FPRINTF("t10%s", after); break;
        case 27: FPRINTF("t11%s", after); break;
        case 28: FPRINTF("sp%s", after); break;
        case 29: FPRINTF("fp%s", after); break;
        case 30: FPRINTF("la%s", after); break;
        case 31: FPRINTF("zero%s", after); break;
    }
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
    uint32_t op = 0;
    int err = dis_info->read_memory_func(addr, the_bytes, 4, dis_info);
    struct favor_dis_info * const info = &the_info;
    if(err) {
        dis_info->memory_error_func(err, addr, dis_info);
        return -1;
    }

    // Create opcode out of bytes.
    op |= the_bytes[0];
    op |= (the_bytes[1] << 8);
    op |= (the_bytes[2] << 16);
    op |= (the_bytes[3] << 24);

    // We read the opcode, disassemble it.
    struct insn insn = favor_decode(op);

    switch(insn.p_opcode) {
        case OP_CC_MISC:
            switch(insn.cc_misc.funct) {
                case CCM_ILL:     pr_opname(info, "ill"); break;
                case CCM_NOP:     pr_opname(info, "nop"); break;
                case CCM_HALT:    pr_opname(info, "halt"); break;
                case CCM_SYSCALL: pr_opname(info, "syscall"); break;
                default: goto bad_op;
            };
            pr_cond(info, insn.cc_misc.conditional);
            break;
        case OP_INT3: {
            uint32_t is_signed = 0;
            switch(insn.int3.funct) {
                case I3_ADD:      pr_opname(info, "add"); break;
                case I3_SUB:      pr_opname(info, "sub"); break;
                case I3_LSH:      pr_opname(info, "lsh"); break;
                case I3_RSHU:     pr_opname(info, "rsh"); is_signed = 0; break;
                case I3_RSHS:     pr_opname(info, "rsh"); is_signed = 1; break;
                case I3_ROL:      pr_opname(info, "rol"); break;
                case I3_ROR:      pr_opname(info, "ror"); break;
                case I3_AND:      pr_opname(info, "and"); break;
                case I3_OR:       pr_opname(info, "or"); break;
                case I3_XOR:      pr_opname(info, "xor"); break;
                case I3_MINS:     pr_opname(info, "min"); is_signed = 1; break;
                case I3_MINU:     pr_opname(info, "min"); is_signed = 0; break;
                case I3_MAXS:     pr_opname(info, "max"); is_signed = 1; break;
                case I3_MAXU:     pr_opname(info, "max"); is_signed = 0; break;
                case I3_LOG_AND:  pr_opname(info, "logand"); break;
                case I3_LOG_OR:   pr_opname(info, "logor"); break;
                case I3_SAT_ADDS: pr_opname(info, "satadd"); is_signed = 1; break;
                case I3_SAT_ADDU: pr_opname(info, "satadd"); is_signed = 0; break;
                case I3_SAT_SUBS: pr_opname(info, "satsub"); is_signed = 1; break;
                case I3_SAT_SUBU: pr_opname(info, "satsub"); is_signed = 0; break;
            }
            pr_type(info, 0, is_signed, insn.int3.sz, insn.int3.vec);
            pr_cond(info, insn.int3.conditional);
            pr_gpr(info, insn.int3.dest, ", ");
            pr_gpr(info, insn.int3.src1, ", ");
            pr_gpr(info, insn.int3.src2, "");

            break;
        }
        case OP_JUMP: {
            switch(insn.jump.funct) {
                case J_JUMP: pr_opname(info, "j"); break;
            }
            pr_cond(info, 0); // TODO conditional
            // TODO: Figure out sign extension? Also...
            FPRINTF("0x");
            dis_info->print_address_func(addr + (sign_extend_32(insn.jump.immediate, 21) * 4), dis_info);
            break;
        }
        case POP_LD_IMM: {
            // Probably we want op to just be a psuedoop, so we keep it like this?
            const char *opname = LOOKUP_OPCODE(ld_imm, insn.ld_imm.funct);
            if(!opname) goto bad_op;

            pr_opname(info, opname);
            pr_cond(info, insn.ld_imm.conditional);
            pr_gpr(info, insn.ld_imm.dest, ", ");
            FPRINTF("0x%x", insn.ld_imm.imm);
            break;
        }
        default:
bad_op:
            FPRINTF(".long 0x%08x", op);
            break;
    }

    return 4; // The number of bytes to advance. -1 on error?
}