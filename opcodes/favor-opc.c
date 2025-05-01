#include "sysdep.h"
#include "opcode/favor.h"

#include <assert.h>

#define RESERVED { .name = NULL }

FAVOR_OP_TABLE_DEFINE(singleton,
    { .parser = PARSE_SINGLETON, .name = "ill",     .funct = SNG_ILL },
    { .parser = PARSE_SINGLETON, .name = "nop",     .funct = SNG_NOP },
    { .parser = PARSE_SINGLETON, .name = "halt",    .funct = SNG_HALT },
    { .parser = PARSE_SINGLETON, .name = "syscall", .funct = SNG_SYSCALL },
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED,
    { .parser = PARSE_SINGLETON, .name = "seteq", .funct = SNG_SETEQ },
    { .parser = PARSE_SINGLETON, .name = "setne", .funct = SNG_SETNE },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setg" , .funct = SNG_SETG,   .type = TYPE_S },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setl",  .funct = SNG_SETL,   .type = TYPE_S },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setge", .funct = SNG_SETGE,  .type = TYPE_S },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setle", .funct = SNG_SETLE,  .type = TYPE_S },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setg",  .funct = SNG_SETGU,  .type = TYPE_U },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setl",  .funct = SNG_SETLU,  .type = TYPE_U },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setge", .funct = SNG_SETGEU, .type = TYPE_U },
    { .parser = PARSE_SINGLETON_TYPED, .name = "setle", .funct = SNG_SETLEU, .type = TYPE_U },
    { .parser = PARSE_SINGLETON, .name = "setneg", .funct = SNG_SETNEG },
    { .parser = PARSE_SINGLETON, .name = "setnne", .funct = SNG_SETNNE },
    RESERVED,
    RESERVED,
    RESERVED,
    RESERVED
)

FAVOR_OP_TABLE_DEFINE(psuedo,
    { .parser = PARSE_PSUEDO_LI, .name = "li" }
)

// For jump opcodes, the and-link is represent by ., e.g. j.l
FAVOR_OP_TABLE_DEFINE(jump,
    { .parser = PARSE_JUMP, .name = "j",    .funct = J_JUMP },
    { .parser = PARSE_JUMP, .name = "beq",  .funct = J_BEQ  },
    { .parser = PARSE_JUMP, .name = "bne",  .funct = J_BNE  },
    { .parser = PARSE_JUMP, .name = "bls",  .funct = J_BL   },
    { .parser = PARSE_JUMP, .name = "bgs",  .funct = J_BG   },
    { .parser = PARSE_JUMP, .name = "bles", .funct = J_BLE  },
    { .parser = PARSE_JUMP, .name = "blu",  .funct = J_BLU  },
    { .parser = PARSE_JUMP, .name = "bgu",  .funct = J_BGU  },
    { .parser = PARSE_JUMP, .name = "bleu", .funct = J_BLEU },
    { .parser = PARSE_JUMP, .name = "bgeu", .funct = J_BGEU },

    { .parser = PARSE_JUMP, .name = "bges", .funct = J_BGE  },
)

FAVOR_OP_TABLE_DEFINE(int3,
    { .parser = PARSE_3ARG, .name = "add",  .funct = I3_ADD,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "sub",  .funct = I3_SUB,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "addc", .funct = I3_ADDC, .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "subc", .funct = I3_SUBC, .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "lsh",  .funct = I3_LSH,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "rsh",  .funct = I3_RSHU, .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "rsh",  .funct = I3_RSHS, .type = TYPE_S },
    { .parser = PARSE_3ARG, .name = "rol",  .funct = I3_ROL,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "ror",  .funct = I3_ROR,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "and",  .funct = I3_AND,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "or",   .funct = I3_OR,   .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "xor",  .funct = I3_XOR,  .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "min",  .funct = I3_MINU, .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "min",  .funct = I3_MINS, .type = TYPE_S },
    { .parser = PARSE_3ARG, .name = "max",  .funct = I3_MAXU, .type = TYPE_U },
    { .parser = PARSE_3ARG, .name = "max",  .funct = I3_MAXS, .type = TYPE_S },
)

FAVOR_OP_TABLE_DEFINE(ld_imm, 
    { .parser = PARSE_REAL_LI, .name = "li0u",   .funct = LDI0U, .type = TYPE_U },
    { .parser = PARSE_REAL_LI, .name = "li0s",   .funct = LDI0S, .type = TYPE_S },
    { .parser = PARSE_REAL_LI, .name = "li0o",   .funct = LDI0O },
    { .parser = PARSE_REAL_LI, .name = "li0opc", .funct = LDI0OPC },

    { .parser = PARSE_REAL_LI, .name = "li1u",   .funct = LDI1U, .type = TYPE_U },
    { .parser = PARSE_REAL_LI, .name = "li1s",   .funct = LDI1S, .type = TYPE_S },
    { .parser = PARSE_REAL_LI, .name = "li1o",   .funct = LDI1O },
    RESERVED,

    { .parser = PARSE_REAL_LI, .name = "li2u",   .funct = LDI2U, .type = TYPE_U },
    { .parser = PARSE_REAL_LI, .name = "li2s",   .funct = LDI2S, .type = TYPE_S },
    { .parser = PARSE_REAL_LI, .name = "li2o",   .funct = LDI2O },
    RESERVED,

    { .parser = PARSE_REAL_LI, .name = "li3u",   .funct = LDI3U, .type = TYPE_U },
    RESERVED,
    { .parser = PARSE_REAL_LI, .name = "li3o",   .funct = LDI3O },
    { .parser = PARSE_REAL_LI, .name = "li0s32", .funct = LDI0S32 },
)

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
            value |= insn.singleton.vec         << 30;
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
                insn.singleton.vec         = code >> 30;
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
                insn.p_opcode = POP_I3;
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
