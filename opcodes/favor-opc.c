#include "sysdep.h"
#include "opcode/favor.h"

#include <assert.h>

#define UF(uv, fv)      .funct_u = uv, .funct_s = -1, .funct_f = fv, .funct_x = -1
#define U(uv)           .funct_u = uv, .funct_s = -1, .funct_f = -1, .funct_x = -1
#define US(uv, sv)      .funct_u = uv, .funct_s = sv, .funct_f = -1, .funct_x = -1
#define USF(uv, sv, fv) .funct_u = uv, .funct_s = sv, .funct_f = fv, .funct_x = -1

/* Table mapping opcode names to data. */
struct favor_op_info
favor_op_table[] = {
    { .name = "add", .opcode = OP_INT_FLOAT_3, UF(I3_ADD, F3_ADD) },
    { .name = "sub", .opcode = OP_INT_FLOAT_3, UF(I3_SUB, F3_SUB) },
    { .name = "lsh", .opcode = OP_INT_FLOAT_3, U (I3_LSH) },
    { .name = "rsh", .opcode = OP_INT_FLOAT_3, US(I3_RSHU, I3_RSHS) },
    { .name = "rol", .opcode = OP_INT_FLOAT_3, U (I3_ROL) },
    { .name = "ror", .opcode = OP_INT_FLOAT_3, U (I3_ROR) },
    { .name = "and", .opcode = OP_INT_FLOAT_3, U (I3_AND) },
    { .name = "or" , .opcode = OP_INT_FLOAT_3, U (I3_OR) },
    { .name = "xor", .opcode = OP_INT_FLOAT_3, U (I3_XOR) },
    { .name = "min", .opcode = OP_INT_FLOAT_3, USF(I3_MINU, I3_MINS, F3_MIN) },
    { .name = "max", .opcode = OP_INT_FLOAT_3, USF(I3_MAXU, I3_MAXS, F3_MAX) },
    { .name = "logand", .opcode = OP_INT_FLOAT_3, U(I3_LOG_AND) },
    { .name = "logor" , .opcode = OP_INT_FLOAT_3, U(I3_LOG_OR) },
    { .name = "satadd", .opcode = OP_INT_FLOAT_3, US(I3_SAT_ADDU, I3_SAT_ADDS) },
    { .name = "satsub", .opcode = OP_INT_FLOAT_3, US(I3_SAT_ADDU, I3_SAT_ADDS) },


    { .name = "ill",     .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_ILL },
    { .name = "nop",     .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_NOP },
    { .name = "halt",    .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_HALT },
    { .name = "syscall", .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_SYSCALL },

    { .name = "j", .opcode = OP_JUMP, .funct_j = J_JUMP },

    { .name = "ld", .opcode = OP_LOAD },
};

FAVOR_OP_TABLE_DEFINE(ld_imm, 
    { .name = "ldi3u", .opcode = POP_LD_IMM, .funct = LDI3U },
    { .name = "ldi3o", .opcode = POP_LD_IMM, .funct = LDI3O },

    { .name = "ldi2s", .opcode = POP_LD_IMM, .funct = LDI2S },
    { .name = "ldi2u", .opcode = POP_LD_IMM, .funct = LDI2U },
    { .name = "ldi2o", .opcode = POP_LD_IMM, .funct = LDI2O },
    
    { .name = "ldi1s", .opcode = POP_LD_IMM, .funct = LDI1S },
    { .name = "ldi1u", .opcode = POP_LD_IMM, .funct = LDI1U },
    { .name = "ldi1o", .opcode = POP_LD_IMM, .funct = LDI1O },
    
    { .name = "ldi0s", .opcode = POP_LD_IMM, .funct = LDI0S },
    { .name = "ldi0u", .opcode = POP_LD_IMM, .funct = LDI0U },
    { .name = "ldi0o", .opcode = POP_LD_IMM, .funct = LDI0O },
    
    { .name = "ldi0opc", .opcode = POP_LD_IMM, .funct = LDI0OPC },
    { .name = "ldi0s32", .opcode = POP_LD_IMM, .funct = LDI0S32 },
)

size_t favor_op_table_size = sizeof(favor_op_table) / sizeof(*favor_op_table);

#define REG(r) .reg = r, .reg_mask = (r & 0x1F), .f = (r >> 5)

struct favor_reg_info favor_reg_table[] = {
    { .name = "a0",  REG(REG_A0) },
    { .name = "a1",  REG(REG_A1) },
    { .name = "a2",  REG(REG_A2) },
    { .name = "a3",  REG(REG_A3) },

    { .name = "a4",  REG(REG_A4) },
    { .name = "a5",  REG(REG_A5) },
    { .name = "a6",  REG(REG_A6) },
    { .name = "a7",  REG(REG_A7) },

    { .name = "a8",  REG(REG_A8) },
    { .name = "a9",  REG(REG_A9) },
    { .name = "a10", REG(REG_A10) },
    { .name = "a11", REG(REG_A11) },

    { .name = "t0",  REG(REG_T0) },
    { .name = "t1",  REG(REG_T1) },
    { .name = "t2",  REG(REG_T2) },
    { .name = "t3",  REG(REG_T3) },

    { .name = "t4",  REG(REG_T4) },
    { .name = "t5",  REG(REG_T5) },
    { .name = "t6",  REG(REG_T6) },
    { .name = "t7",  REG(REG_T7) },

    { .name = "t8",  REG(REG_T8) },
    { .name = "t9",  REG(REG_T9) },
    { .name = "t10", REG(REG_T10) },
    { .name = "t11", REG(REG_T11) },

    { .name = "r0",  REG(REG_R0) },
    { .name = "r1",  REG(REG_R1) },
    { .name = "r2",  REG(REG_R2) },
    { .name = "r3",  REG(REG_R3) },

    { .name = "fp",  REG(REG_FP) },
    { .name = "la",  REG(REG_LA) },
    { .name = "sp",  REG(REG_SP) },
    { .name = "zero", REG(REG_ZERO) },
};
size_t favor_reg_table_size = sizeof(favor_reg_table) / sizeof(*favor_reg_table);

enum {
    VARIANT_0 = 0,
    VARIANT_1 = 0x10
};

uint32_t
favor_encode(struct insn insn) {
    uint32_t value = 0;
    if(insn.p_opcode < 16) {
        // Psuedo instructions will have to set the opcode explicitly.
        value |= (insn.p_opcode);
    }

    switch(insn.p_opcode) {
        case POP_SINGLETON: {
            value |= OP_MISC;
            value |= insn.singleton.is_return   << 4;
            value |= insn.singleton.conditional << 6;
            value |= (insn.singleton.funct & 0x1F) << 7;
            value |= (insn.singleton.funct >> 5)   << 17;
            // Note that the misc funct field is 0, but that's already done.
            break;
        }
        case OP_MISC: {
            value |= insn.misc.is_return   << 4 ;
            value |= insn.misc.conditional << 6 ;
            value |= insn.misc.dest        << 7 ;
            value |= insn.misc.arg_r       << 17;
            value |= insn.misc.arg_a       << 23;
            value |= insn.misc.vec         << 30;

            value |= (insn.misc.funct & 1)  << 5;
            value |= (insn.misc.funct >> 1) << 12;
            break;
        }
        case OP_JUMP: {
            value |= insn.jump.and_link  << 4;
            value |= insn.jump.immediate << 6;

            value |= (insn.jump.funct & 1)  << 5;
            value |= (insn.jump.funct >> 1) << 28;
            break;
        }
        case POP_I3: {
            value |= OP_INT;
            value |= VARIANT_0;
            value |= insn.int3.replication << 5 ;
            value |= insn.int3.conditional << 6 ;
            value |= insn.int3.dest        << 7 ;
            value |= insn.int3.src2        << 12;
            value |= insn.int3.src1        << 17;
            value |= insn.int3.funct       << 22;
            value |= insn.int3.sz          << 28;
            value |= insn.int3.vec         << 30;
            break;
        }
        case POP_I2: {
            value |= OP_INT;
            value |= VARIANT_1;
            value |= insn.int2.replication << 5 ;
            value |= insn.int2.conditional << 6 ;
            value |= insn.int2.dest        << 7 ;
            value |= insn.int2.src2        << 12;
            value |= insn.int2.funct       << 17;
            value |= 0x1D                  << 23;
            value |= insn.int2.sz          << 28;
            value |= insn.int2.vec         << 30;
            break;
        }
        case POP_FIX2: {
            value |= OP_INT;
            value |= VARIANT_1;
            value |= insn.fix2.replication << 5 ;
            value |= insn.fix2.conditional << 6 ;
            value |= insn.fix2.dest        << 7 ;
            value |= insn.fix2.src2        << 12;
            value |= insn.fix2.shift       << 17;
            value |= insn.fix2.funct       << 23;
            assert(insn.fix2.funct < 0x1D);
            value |= insn.fix2.sz          << 28;
            value |= insn.fix2.vec         << 30;
            break;
        }
        case POP_INT_SWIZZLE: {
            value |= OP_INT;
            value |= VARIANT_1;
            value |= insn.swizzle.conditional << 6 ;
            value |= insn.swizzle.dest        << 7 ;
            value |= insn.swizzle.src2        << 12;

            value |= (insn.swizzle.a & 1)  << 5;
            value |= (insn.swizzle.a >> 1) << 17;

            value |= insn.swizzle.b           << 18;
            value |= insn.swizzle.c           << 20;
            value |= insn.swizzle.d           << 22;
            value |= 0xF                      << 24;
            value |= insn.swizzle.sz          << 28;
            value |= insn.swizzle.vec         << 30;
            break;
        }
        case POP_F3: {
            break;
        }
        case POP_F2: {
            break;
        }
        case POP_FLOAT_SWIZZLE: {
            break;
        }
        case OP_LOAD:
        case OP_STORE: {
            value |= VARIANT_0;
            value |= insn.ls.fp          << 5 ;
            value |= insn.ls.conditional << 6 ;
            value |= insn.ls.dest        << 7 ;
            value |= insn.ls.src2        << 12;
            value |= insn.ls.src1        << 17;
            value |= insn.ls.shift       << 22;
            value |= insn.ls.offset      << 25;
            value |= insn.ls.sz          << 28;
            value |= insn.ls.vec         << 30;
            break;
        }
        case POP_LOAD_LONG:
        case POP_STORE_LONG: {
            value |= (insn.p_opcode == POP_LOAD_LONG) ? OP_LOAD : OP_STORE;
            value |= VARIANT_1;
            value |= insn.ls_long.fp          << 5 ;
            value |= insn.ls_long.conditional << 6 ;
            value |= insn.ls_long.dest        << 7 ;
            value |= insn.ls_long.src2        << 12;
            value |= insn.ls_long.offset      << 17;
            value |= insn.ls_long.sz          << 28;
            value |= insn.ls_long.vec         << 30;
            break;
        }
        case OP_LS_SPECIAL: {
            value |= VARIANT_0;
            value |= insn.ls_special.fp          << 5 ;
            value |= insn.ls_special.conditional << 6 ;
            value |= insn.ls_special.dest        << 7 ;
            value |= insn.ls_special.funct       << 12;
            value |= insn.ls_special.imm         << 16;
            value |= insn.ls_special.sz          << 28;
            value |= insn.ls_special.vec         << 30;
            break;
        }
        case POP_LD_IMM: {
            value |= OP_LS_SPECIAL;
            value |= VARIANT_1;
            value |= insn.ld_imm.fp          << 5 ;
            value |= insn.ld_imm.conditional << 6 ;
            value |= insn.ld_imm.dest        << 7 ;
            value |= insn.ld_imm.imm         << 12;
            value |= insn.ld_imm.funct       << 28;
            break;
        }
        case OP_INT_IMM: {
            value |= VARIANT_0;
            value |= insn.int_imm.conditional << 6 ;
            value |= insn.int_imm.dest        << 7 ;
            
            value |= (insn.int_imm.funct & 1)  << 5;
            value |= (insn.int_imm.funct >> 1) << 12;

            value |= insn.int_imm.imm         << 16;
            value |= insn.int_imm.sz          << 28;
            value |= insn.int_imm.vec         << 30;
            break;
        }
        case POP_INT_IMM_ADDSUB: {
            value |= OP_INT_IMM;
            value |= VARIANT_1;
            value |= insn.int_imm_addsub.funct       << 5 ;
            value |= insn.int_imm_addsub.conditional << 6 ;
            value |= insn.int_imm_addsub.dest        << 7 ;
            value |= insn.int_imm_addsub.imm         << 12;
            value |= insn.int_imm_addsub.sz          << 28;
            value |= insn.int_imm_addsub.vec         << 30;
            break;
        }
    }

    return value;
}

struct insn
favor_decode(uint32_t code) {
    struct insn insn;

    // Do bitfields automatically get masked out? Convenient if true.

    // NOTE: We must manually mask out p_opcode as we start with the real opcode,
    // which is not the bit field width.
    insn.p_opcode = code & 0xF;
    uint32_t variant = (code >> 4) & 1;

    switch(insn.p_opcode) {
        case OP_MISC: {
            uint32_t funct =
                 ((code >>  5) & 0x01)       |
                (((code >> 12) & 0x1F) << 1);
            if(funct == 0) {
                /* Singleton -- funct == 0 */
                insn.p_opcode = POP_SINGLETON;
                insn.singleton.is_return   = code >> 4;
                insn.singleton.conditional = code >> 6;
                insn.singleton.funct =
                    ((code >>  7) & 0x1F) |
                    ((code >> 17) << 5);
            }
            else {
                /* Non-singleton */
                insn.misc.is_return   = code >> 4 ;
                insn.misc.conditional = code >> 6 ;
                insn.misc.dest        = code >> 7 ;
                insn.misc.arg_r       = code >> 17;
                insn.misc.arg_a       = code >> 23;
                insn.misc.vec         = code >> 30;
                insn.misc.funct       = funct;
            }
            break;
        }
        case OP_JUMP:
            insn.jump.funct     = ((code >> 5) & 1) | (((code >> 28) & 0xF) << 1);
            insn.jump.and_link  = code >> 4;
            insn.jump.immediate = code >> 6;
            break;
        case OP_INT: {
            // Variant instructions are either swizzle, 2-arg int, or 2-arg fix.
            if(variant) {
                uint32_t bigfunct = (code >> 17) & 0x5FF;
                if((bigfunct >> 7) == 0xF) {
                    // Swizzle instruction.
                    insn.p_opcode = POP_INT_SWIZZLE;
                    insn.swizzle.conditional = code >> 6 ;
                    insn.swizzle.dest        = code >> 7 ;
                    insn.swizzle.src2        = code >> 12;
                    insn.swizzle.a           = ((code >> 5) & 1) | (((code >> 17) & 1) << 1);
                    insn.swizzle.b           = code >> 18;
                    insn.swizzle.c           = code >> 20;
                    insn.swizzle.d           = code >> 22;
                    insn.swizzle.sz          = code >> 28;
                    insn.swizzle.vec         = code >> 30;
                }
                else if((bigfunct >> 6) == 0x1D) {
                    // 2-arg integer instruction.
                    insn.p_opcode = POP_I2;
                    insn.int2.replication = code >> 5 ;
                    insn.int2.conditional = code >> 6 ;
                    insn.int2.dest        = code >> 7 ;
                    insn.int2.src2        = code >> 12;
                    insn.int2.funct       = bigfunct;
                    insn.int2.sz          = code >> 28;
                    insn.int2.vec         = code >> 30;
                }
                else {
                    // Fixed-point instruction.
                    insn.p_opcode = POP_FIX2;
                    insn.fix2.replication = code >> 5 ;
                    insn.fix2.conditional = code >> 6 ;
                    insn.fix2.dest        = code >> 7 ;
                    insn.fix2.src2        = code >> 12;
                    insn.fix2.shift       = code >> 17;
                    insn.fix2.funct       = bigfunct >> 6;
                    insn.fix2.sz          = code >> 28;
                    insn.fix2.vec         = code >> 30;
                }
            }
            else {
                insn.int3.replication = code >> 5 ;
                insn.int3.conditional = code >> 6 ;
                insn.int3.dest        = code >> 7 ;
                insn.int3.src2        = code >> 12;
                insn.int3.src1        = code >> 17;
                insn.int3.funct       = code >> 22;
                insn.int3.sz          = code >> 28;
                insn.int3.vec         = code >> 30;
            }
            break;
        }
        case OP_FLOAT: {
            // TODO
            break;
        }

        case OP_LOAD:
        case OP_STORE: {
            if(!variant) {
                // Variant 0: has src1 arg & shorter offset
                insn.ls.fp          = code >> 5 ;
                insn.ls.conditional = code >> 6 ;
                insn.ls.dest        = code >> 7 ;
                insn.ls.src2        = code >> 12;
                insn.ls.src1        = code >> 17;
                insn.ls.shift       = code >> 22;
                insn.ls.offset      = code >> 25;
                insn.ls.sz          = code >> 28;
                insn.ls.vec         = code >> 30;
            }
            else {
                // Variant 1: has long offset
                insn.p_opcode = (insn.p_opcode == OP_LOAD) ? POP_LOAD_LONG : POP_STORE_LONG;

                insn.ls_long.fp          = code >> 5 ;
                insn.ls_long.conditional = code >> 6 ;
                insn.ls_long.dest        = code >> 7 ;
                insn.ls_long.src2        = code >> 12;
                insn.ls_long.offset      = code >> 17;
                insn.ls_long.sz          = code >> 28;
                insn.ls_long.vec         = code >> 30;
            }
            
            break;
        }
        case OP_LS_SPECIAL: {
            if(!variant) {
                // Variant 0: Main set of extra ops
                insn.ls_special.fp          = code >> 5 ;
                insn.ls_special.conditional = code >> 6 ;
                insn.ls_special.dest        = code >> 7 ;
                insn.ls_special.funct       = code >> 12;
                insn.ls_special.imm         = code >> 16;
                insn.ls_special.sz          = code >> 28;
                insn.ls_special.vec         = code >> 30;
            }
            else {
                // Variant 1: Immediate loads
                insn.p_opcode = POP_LD_IMM;

                insn.ld_imm.fp          = code >> 5 ;
                insn.ld_imm.conditional = code >> 6 ;
                insn.ld_imm.dest        = code >> 7 ;
                insn.ld_imm.imm         = code >> 12;
                insn.ld_imm.funct       = code >> 28;
            }
            break;
        }

        case OP_INT_IMM: {
            if(!variant) {
                // Variant 0: general immediate ops
                insn.int_imm.conditional = code >> 6 ;
                insn.int_imm.dest        = code >> 7 ;
                insn.int_imm.funct       = 
                     ((code >> 5) & 1) |
                    (((code >> 12) & 0xF) << 1);
                insn.int_imm.imm         = code >> 16;
                insn.int_imm.sz          = code >> 28;
                insn.int_imm.vec         = code >> 30;
            }
            else {
                // Variant 1: add/sub immediate ops
                insn.p_opcode = POP_INT_IMM_ADDSUB;
                insn.int_imm_addsub.funct       = code >> 5 ;
                insn.int_imm_addsub.conditional = code >> 6 ;
                insn.int_imm_addsub.dest        = code >> 7 ;
                insn.int_imm_addsub.imm         = code >> 12;
                insn.int_imm_addsub.sz          = code >> 28;
                insn.int_imm_addsub.vec         = code >> 30;
            }
            break;
        }
    }

    return insn;
}
