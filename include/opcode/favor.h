#ifndef OPCODE_FAVOR_H
#define OPCODE_FAVOR_H

/* opcodes for the FAVOR isa. */

#include <stdint.h>

struct favor_op_info {
    const char *name;

    uint32_t parser;
    uint32_t funct;
    uint32_t type;

    struct favor_op_info *next;
};

enum favor_op_parse {
    PARSE_3ARG,
    PARSE_2ARG,
    PARSE_PSUEDO_LI,
    PARSE_REAL_LI,
    PARSE_SINGLETON,
    PARSE_SINGLETON_TYPED,
    PARSE_JUMP,
    PARSE_PSUEDO_RET,
};

enum favor_op_type {
    TYPE_NONE,
    TYPE_U,
    TYPE_S,
    TYPE_F,
    TYPE_R,
};

struct favor_reg_info {
    const char *name;
    uint32_t reg: 6;
    uint32_t reg_mask: 5;
    uint32_t f: 1;
};

#define FAVOR_OP_TABLE_DECLARE(name) \
extern struct favor_op_info favor_op_ ## name[]; \
extern size_t favor_op_ ## name ## _count; \

#define FAVOR_OP_TABLE_DEFINE(name, ...) \
struct favor_op_info favor_op_ ## name[] = { \
    __VA_ARGS__ \
}; \
size_t favor_op_ ## name ## _count = sizeof(favor_op_ ## name) / sizeof(*favor_op_ ## name);

FAVOR_OP_TABLE_DECLARE(singleton)
FAVOR_OP_TABLE_DECLARE(jump)
FAVOR_OP_TABLE_DECLARE(int3)
FAVOR_OP_TABLE_DECLARE(int2)
FAVOR_OP_TABLE_DECLARE(ld_imm)

FAVOR_OP_TABLE_DECLARE(psuedo)

extern struct favor_op_info favor_op_table[];
extern size_t favor_op_table_size;

extern struct favor_reg_info favor_reg_table[];
extern size_t favor_reg_table_size;

// extern struct favor_ty_info favor_ty_table[];
// extern size_t favor_ty_table_size;

/* Instruction kind: 0 arg, 1 arg, 2 arg, 3 arg */
enum opcode {
    /* MISC instructions. Corresponds to .misc */
    OP_MISC,
    /* JUMP instructions. Corresponds to .jump */
    OP_JUMP,
    /* INT instructions. All of these use POP codes. */
    OP_INT,
    /* FLOAT instructions. All of these use POP codes. */
    OP_FLOAT,
    /* LOAD instructions. Corresponds to .ls */
    OP_LOAD,
    /* STORE instructions. Corresponds to .ls */
    OP_STORE,
    /* Corresponds to .ls_special */
    OP_LS_SPECIAL,
    /* Corresponds to .int_imm */
    OP_INT_IMM,

    /* Psuedo-opcodes:
     * These do not fit in the actual 4-bit opcode field of the instruction.
     * Instead, they indicate sub-types of the struct insn to make working
     * with various opcodes easier, such as ld_imm opcodes. */
    POP_START = 16,

    /* Corresponds to .singleton */
    POP_SINGLETON,
    
    /* Three-arg, two-arg integer instructions */
    /* Corresponds to .int3 */
    POP_I3,
    /* Corresponds to .int2 */
    POP_I2,

    /* Fixed-point instructions. Corresponds to .fix2 */
    POP_FIX2,

    /* Swizzle instructions. Corresponds to .swizzle */
    POP_INT_SWIZZLE,
    POP_FLOAT_SWIZZLE,

    /* Three-arg, two-arg floating point instructions */
    /* Corresponds to .float3 */
    POP_F3,
    /* Corresponds to .float2 */
    POP_F2,

    /* Corresponds to .ls_long */
    POP_LOAD_LONG,
    /* Corresponds to .ls_long */
    POP_STORE_LONG,
    
    /* Corresponds to .ld_imm */
    POP_LD_IMM,

    /* Corresponds to .int_imm_addsub */
    POP_INT_IMM_ADDSUB,
};

/**
 * Not real opcodes, but or'd in with the opcode field in favor_op_info to
 * help narrow down certain things.
 */
enum opcode_flag {
    OP_CC_MISC_SINGLETON = 0x100 | OP_MISC,
    OP_INT_FLOAT_3 = 0x200,
};

/**
 * Conditions--ways to compute a single boolean result based on the status flags.
 * Always encoded in the same order (and ideally in the same bit position).
 */
enum condition {
    COND_EQ,
    COND_NE,
    
    // For le/g and l/ge, note that each one is the opposite of the other
    // with respect to the least-significant bit.
    COND_LEU,
    COND_GU,
    COND_LU,
    COND_GEU,
    
    COND_LE,
    COND_G,
    COND_L,
    COND_GE,

    COND_NEG,
    // really non-negative: todo better name?
    COND_NNE,
};

enum cc_misc {
    CCM_SINGLETON = 0,
    CCM_TEST, /* Test the dest register, set C = 1 if the bit is set */
    CCM_READC,
    CCM_WRITEC,
    CCM_WRITEC_IMM,
};

enum singleton {
    SNG_ILL,
    SNG_NOP,
    SNG_HALT,
    SNG_SYSCALL,

    SNG_SETEQ = 16,
    SNG_SETNE,

    SNG_SETLEU,
    SNG_SETGU,
    SNG_SETLU,
    SNG_SETGEU,

    SNG_SETLE,
    SNG_SETG,
    SNG_SETL,
    SNG_SETGE,
    
    SNG_SETNEG,
    SNG_SETNNE,
    SNG_SETRES0,
    SNG_SETRES1,
    SNG_SETRES2,
    SNG_SETRES3,
};

enum jump {
    JC_JUMP,
    JC_BEQ,
    JC_BNE,
    
    JC_BLEU,
    JC_BGU,
    JC_BLU,
    JC_BGEU,
    JC_BLE,
    JC_BG,
    JC_BL,

    JIC_JUMP,
    JIC_BEQ,
    JIC_BNE,
    
    JIC_BLUE,
    JIC_BGU,
    JIC_BLU,
    JIC_BGEU,
    JIC_BLE,
    JIC_BG,
    JIC_BL,

    J_JUMP,
    J_BEQ,
    J_BNE,
    
    J_BLEU,
    J_BGU,
    J_BLU,
    J_BGEU,
    J_BLE,
    J_BG,
    J_BL,

    // Unconditional-only.
    J_BGE,
};

enum int3 {
    I3_ADD,
    I3_SUB,
    I3_ADDC,
    I3_SUBC,
    I3_LSH,
    I3_RSHU,
    I3_RSHS,
    I3_ROL,
    I3_ROR,
    I3_AND,
    I3_OR,
    I3_XOR,
    I3_MINU,
    I3_MINS,
    I3_MAXU,
    I3_MAXS,
    // I3_LOG_AND,
    // I3_LOG_OR,
    // I3_SAT_ADDS,
    // I3_SAT_ADDU,
    // I3_SAT_SUBS,
    // I3_SAT_SUBU
};

enum int2 {
    I2_CMP_EQ,
    I2_CMP_NE,
    
    I2_CMP_LEU,
    I2_CMP_GU,
    I2_CMP_LU,
    I2_CMP_GEU,
    
    I2_CMP_LE,
    I2_CMP_G,
    I2_CMP_L,
    I2_CMP_GE,
    

    I2_CMP_NEG,
    // really non-negative: todo better name?
    I2_CMP_NNE,

    I2_CMP_RES0,
    I2_CMP_RES1,
    I2_CMP_RES2,
    I2_CMP_RES3,

    I2_CMP,
    I2_NEGATE,
    I2_NOT,
    
};

enum float3 {
    F3_ADD,
    F3_SUB,
    F3_MIN,
    F3_MAX,
    F3_MUL,
    F3_DIV,
    
    F3_NORM,
    F3_LENGTH,
    F3_DOT,
};

enum float2 {
    F2_NEGATE,
    F2_CMP,
    F2_SWIZZLE,
};

/**
 * Opcodes for the li (load immediate) instructions.
 * 
 * These are organized such that if we have a U and S version of an
 * instruction, then the S version equals the U version | 1 (in the funct
 * field).
 * 
 * This allows us to easily implement relocations for these, as well as in
 * theory could be convenient in hardware.
 */
enum ld_imm {
    LDI0U,
    LDI0S,

    LDI0O,
    LDI0OPC,

    LDI1U,
    LDI1S,

    LDI1O,
    LDI_RES0,

    LDI2U,
    LDI2S,

    LDI2O,
    LDI_RES1,

    LDI3U,
    LDI_RES2,
    LDI3O,
    LDI0S32,
};

enum gpr {
    REG_A0,
    REG_A1,
    REG_A2,
    REG_A3,

    REG_A4,
    REG_A5,
    REG_A6,
    REG_A7,

    REG_A8,
    REG_A9,
    REG_A10,
    REG_A11,

    REG_T0,
    REG_T1,
    REG_T2,
    REG_T3,

    REG_T4,
    REG_T5,
    REG_T6,
    REG_T7,
    
    REG_T8,
    REG_T9,
    REG_T10,
    REG_T11,

    REG_R0,
    REG_R1,
    REG_R2,
    REG_R3,

    REG_FP,
    REG_LA,
    REG_SP,
    REG_ZERO
};

/* TODO:
 * Consider combining int3 + int2 into one chunk, and
 * float3 + float2 into one chunk. */

/**
 * Helper struct for easily encoding / decoding instructions.
 */
struct insn {
    /**
     * Psuedo-opcode.
     * 
     * This field is either one of the OP_ values, in which case it should generally
     * be encoded exactly as is into the opcode field of the instruction. But,
     * it can also be one of the POP_ values, in which case it doesn't represent
     * a real opcode.
     * 
     * In practice, this field is the discriminant of the union.
     */
    uint32_t p_opcode;

    union {
        struct {
            uint32_t is_return : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            /* non-zero */
            uint32_t funct : 6;
            uint32_t arg_r : 6;
            uint32_t arg_a : 7;
            uint32_t vec : 2;
        } misc;

        struct {
            uint32_t is_return : 1;
            uint32_t conditional : 1;
            uint32_t funct : 18;
            uint32_t vec : 2;
        } singleton;

        struct {
            uint32_t and_link : 1;
            uint32_t funct : 5;
            uint32_t immediate : 22;
        } jump;

        struct {
            uint32_t replication : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t src1 : 5;
            uint32_t funct : 6; 
            uint32_t sz : 2;
            uint32_t vec : 2;
        } int3;

        struct {
            uint32_t replication : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t funct : 6; 
            uint32_t sz : 2;
            uint32_t vec : 2;
        } int2;

        struct {
            uint32_t replication : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t shift : 6;
            uint32_t funct : 5; 
            uint32_t sz : 2;
            uint32_t vec : 2;
        } fix2;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t a : 2;
            uint32_t b : 2;
            uint32_t c : 2;
            uint32_t d : 2;
            /** NOTE: For FP, this is either 0 or 1. */
            uint32_t sz : 2;

            uint32_t vec : 2;
        } swizzle;

        struct {
            uint32_t replication : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t src1 : 5;
            uint32_t funct : 7; 
            uint32_t sz : 2;
            uint32_t vec : 2;
        } float3;

        struct {
            uint32_t replication : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t funct : 12; 
            uint32_t sz : 2;
            uint32_t vec : 2;
        } float2;

        struct {
            uint32_t fp : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t src1 : 5;
            uint32_t shift : 3;
            uint32_t offset : 3;
            uint32_t sz : 2;
            uint32_t vec : 2;
        } ls; /* load-store */

        struct {
            uint32_t fp : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src2 : 5;
            uint32_t offset : 11;
            uint32_t sz : 2;
            uint32_t vec : 2;
        } ls_long;

        struct {
            uint32_t fp : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t imm : 16;
            uint32_t funct: 4;
        } ld_imm;

        struct {
            uint32_t fp : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t funct : 4;
            uint32_t imm : 12;
            uint32_t sz : 2;
            uint32_t vec : 2;
        } ls_special;

        struct {
            uint32_t funct : 1;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t imm : 16;
            uint32_t sz : 2;
            uint32_t vec : 2;
        } int_imm_addsub;

        struct {
            uint32_t funct : 5;
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t imm : 12;
            uint32_t sz : 2;
            uint32_t vec : 2;
        } int_imm;
    };
};

static inline
struct insn
mk_singleton(uint32_t conditional, uint32_t funct, uint32_t is_return) {
    struct insn result = {0};
    result.p_opcode = POP_SINGLETON;
    result.singleton.conditional = conditional;
    result.singleton.funct = funct;
    result.singleton.is_return = is_return;
    return result;
}

static inline
struct insn
mk_int3(uint32_t replication, uint32_t conditional, uint32_t dest, uint32_t src1, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = POP_I3;
    result.int3.replication = replication;
    result.int3.conditional = conditional;
    result.int3.dest = dest;
    result.int3.src1 = src1;
    result.int3.src2 = src2;
    result.int3.sz = sz;
    result.int3.vec = vec;
    result.int3.funct = funct;
    return result;
}

static inline
struct insn
mk_int2(uint32_t replication, uint32_t conditional, uint32_t dest, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = POP_I2;
    result.int2.replication = replication;
    result.int2.conditional = conditional;
    result.int2.dest = dest;
    result.int2.src2 = src2;
    result.int2.sz = sz;
    result.int2.vec = vec;
    result.int2.funct = funct;
    return result;
}

static inline
struct insn
mk_float3(uint32_t replication, uint32_t conditional, uint32_t dest, uint32_t src1, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = POP_F3;
    result.float3.replication = replication;
    result.float3.conditional = conditional;
    result.float3.dest = dest;
    result.float3.src1 = src1;
    result.float3.src2 = src2;
    result.float3.sz = sz;
    result.float3.vec = vec;
    result.float3.funct = funct;
    return result;
}

static inline
struct insn
mk_float2(uint32_t replication, uint32_t conditional, uint32_t dest, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = POP_F2;
    result.float2.replication = replication;
    result.float2.conditional = conditional;
    result.float2.dest = dest;
    result.float2.src2 = src2;
    result.float2.sz = sz;
    result.float2.vec = vec;
    result.float2.funct = funct;
    return result;
}

static inline
struct insn
mk_ld_imm(uint32_t conditional, uint32_t dest, uint32_t imm, uint32_t fp, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = POP_LD_IMM;
    result.ld_imm.conditional = conditional;
    result.ld_imm.dest = dest;
    result.ld_imm.fp = fp;
    result.ld_imm.funct = funct;
    result.ld_imm.imm = imm;
    return result;
}

uint32_t favor_encode(struct insn insn);
struct insn favor_decode(uint32_t code);


#endif