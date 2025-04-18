#include "sysdep.h"
#include <stdio.h>

#include "disassemble.h"

// TODO: Fgure out if we want these options.
#define STATIC_TABLE
#define DEFINE_TABLE

#include "opcode/favor.h"
#include "dis-asm.h"

static const char*
lookup_opcode(struct favor_op_info *table, size_t size, uint32_t funct) {
    if(funct < size) {
        return table[funct].name;
    }
    return NULL;
}

#define LOOKUP_OPCODE(table, funct) lookup_opcode(favor_op_ ## table, favor_op_ ## table ## _count, funct)

static void
pr_cond(fprintf_ftype pr, void *stream, uint32_t conditional) {
    if(conditional) {
        pr(stream, "?\t");
    }
    else {
        pr(stream, " \t");
    }
}

static void
pr_opname(fprintf_ftype pr, void *stream, const char *opname, uint32_t conditional) {
    pr(stream, "%s", opname);
    pr_cond(pr, stream, conditional);
}

static void
pr_type(fprintf_ftype pr, void *stream, uint32_t fp, uint32_t is_signed, uint32_t sz, uint32_t vec) {
    pr(stream, ".");

    if(fp) pr(stream, "f");
    else if(is_signed) { pr(stream, "s"); }
    else { pr(stream, "u"); }

    switch(sz) {
        case 0: pr(stream, "8"); break;
        case 1: pr(stream, "16"); break;
        case 2: pr(stream, "32"); break;
        case 3: pr(stream, "64"); break;
    }
    switch(vec) {
        case 0: break;
        case 1: pr(stream, "x2"); break;
        case 2: pr(stream, "x3"); break;
        case 3: pr(stream, "x4"); break;
    }
}

static void
pr_gpr(fprintf_ftype pr, void *stream, uint32_t reg, const char *after) {
    switch(reg) {
        case 0: pr(stream, "a0%s", after); break;
        case 1: pr(stream, "a1%s", after); break;
        case 2: pr(stream, "a2%s", after); break;
        case 3: pr(stream, "a3%s", after); break;
        case 4: pr(stream, "a4%s", after); break;
        case 5: pr(stream, "a5%s", after); break;
        case 6: pr(stream, "a6%s", after); break;
        case 7: pr(stream, "a7%s", after); break;
        case 8: pr(stream, "v0%s", after); break;
        case 9: pr(stream, "v1%s", after); break;
        case 10: pr(stream, "v2%s", after); break;
        case 11: pr(stream, "v3%s", after); break;
        case 12: pr(stream, "v4%s", after); break;
        case 13: pr(stream, "v5%s", after); break;
        case 14: pr(stream, "v6%s", after); break;
        case 15: pr(stream, "v7%s", after); break;
        case 16: pr(stream, "t0%s", after); break;
        case 17: pr(stream, "t1%s", after); break;
        case 18: pr(stream, "t2%s", after); break;
        case 19: pr(stream, "t3%s", after); break;
        case 20: pr(stream, "t4%s", after); break;
        case 21: pr(stream, "t5%s", after); break;
        case 22: pr(stream, "t6%s", after); break;
        case 23: pr(stream, "t7%s", after); break;
        case 24: pr(stream, "t8%s", after); break;
        case 25: pr(stream, "t9%s", after); break;
        case 26: pr(stream, "t10%s", after); break;
        case 27: pr(stream, "t11%s", after); break;
        case 28: pr(stream, "sp%s", after); break;
        case 29: pr(stream, "fp%s", after); break;
        case 30: pr(stream, "la%s", after); break;
        case 31: pr(stream, "zero%s", after); break;
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
print_insn_favor(bfd_vma addr, struct disassemble_info *info) {
    fprintf_ftype pr = info->fprintf_func;
    void *stream = info->stream;

    bfd_byte the_bytes[4];
    uint32_t op = 0;
    int err = info->read_memory_func(addr, the_bytes, 4, info);
    if(err) {
        info->memory_error_func(err, addr, info);
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
                case CCM_ILL: pr(stream, "ill"); break;
                case CCM_NOP: pr(stream, "nop"); break;
                case CCM_HALT: pr(stream, "halt"); break;
                case CCM_SYSCALL: pr(stream, "syscall"); break;
                default:
                    pr(stream, "(bad)");
                    break;
            };
            pr_cond(pr, stream, insn.cc_misc.conditional);
            break;
        case OP_INT3: {
            uint32_t is_signed = 0;
            switch(insn.int3.funct) {
                case I3_ADD: pr(stream, "add"); break;
                case I3_SUB: pr(stream, "sub"); break;
                case I3_LSH: pr(stream, "lsh"); break;
                case I3_RSHU: pr(stream, "rsh"); is_signed = 0; break;
                case I3_RSHS: pr(stream, "rsh"); is_signed = 1; break;
                case I3_ROL: pr(stream, "rol"); break;
                case I3_ROR: pr(stream, "ror"); break;
                case I3_AND: pr(stream, "and"); break;
                case I3_OR: pr(stream, "or"); break;
                case I3_XOR: pr(stream, "xor"); break;
                case I3_MINS: pr(stream, "min"); is_signed = 1; break;
                case I3_MINU: pr(stream, "min"); is_signed = 0; break;
                case I3_MAXS: pr(stream, "max"); is_signed = 1; break;
                case I3_MAXU: pr(stream, "max"); is_signed = 0; break;
                case I3_LOG_AND: pr(stream, "logand"); break;
                case I3_LOG_OR: pr(stream, "logor"); break;
                case I3_SAT_ADDS: pr(stream, "satadd"); is_signed = 1; break;
                case I3_SAT_ADDU: pr(stream, "satadd"); is_signed = 0; break;
                case I3_SAT_SUBS: pr(stream, "satsub"); is_signed = 1; break;
                case I3_SAT_SUBU: pr(stream, "satsub"); is_signed = 0; break;
            }
            pr_type(pr, stream, 0, is_signed, insn.int3.sz, insn.int3.vec);
            pr_cond(pr, stream, insn.int3.conditional);
            pr_gpr(pr, stream, insn.int3.dest, ", ");
            pr_gpr(pr, stream, insn.int3.src1, ", ");
            pr_gpr(pr, stream, insn.int3.src2, "");

            break;
        }
        case OP_JUMP: {
            switch(insn.jump.funct) {
                case J_JUMP: pr(stream, "j     "); break;
            }
            pr_cond(pr, stream, 0); // TODO conditional
            // TODO: Figure out sign extension? Also...
            pr(stream, "\t0x");
            info->print_address_func(addr + (sign_extend_32(insn.jump.immediate, 21) * 4), info);
            break;
        }
        case POP_LD_IMM: {
            // Probably we want op to just be a psuedoop, so we keep it like this?
            const char *opname = LOOKUP_OPCODE(ld_imm, insn.ld_imm.funct);
            if(!opname) goto bad_op;

            pr_opname(pr, stream, opname, insn.ld_imm.conditional);
            pr_gpr(pr, stream, insn.ld_imm.dest, ", ");
            pr(stream, "0x%x", insn.ld_imm.imm);
            break;
        }
        default:
bad_op:
            pr(stream, "(bad)");
            break;
    }

    return 4; // The number of bytes to advance. -1 on error?
}