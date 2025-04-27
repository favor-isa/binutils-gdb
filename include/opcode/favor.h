#ifndef OPCODE_FAVOR_H
#define OPCODE_FAVOR_H

/* opcodes for the FAVOR isa. */

#include <stdint.h>

/* New(er) instruction type ideas:
 * - 0 argument (halt, nop, unconditional jump, conditional jump)
 * - 1 argument (add immediate (?), rotate (?))
 *     - these might be needed due to bit availability constraints. a += 3,
 *       cause we can't easily do a = b + 3.
 * - 2 argument (swizzle, compute length, negate, invert) 
 *     - these often correspond to unary operations.  a = -b
 * - 3 argument (add a, b, c) 
 *     - these often correspond to binary operations. a = x + y
 * To distinguish between these we need at least 2 bits. But, we can also do
 * rle nonsense if we like.
 * That said it seems like just dedicating 2 bits to this might be best.
 * 
 * size field:
 * - 1 byte signed
 * - 2 byte signed
 * - 4 byte signed
 * - 8 byte signed
 * - 1 byte unsigned
 * - 2 byte unsigned
 * - 4 byte unsigned
 * - 8 byte unsigned
 * - 4 byte float
 * - 8 byte float
 * 12 options... need 4 bits. So we could also just do [F][U][SZ]
 * 
 * Fun idea for C flag:
 * So we want some kind of condition code flag that is used for the C instructions.
 * But we really should have this flag equal to one of the specific condition
 * codes in most cases. For example, we could say it's equal to the nonzero flag
 * or maybe to the greater-than flag or something.
 * 
 * That way, when you have, say, a loop, you can encode the continue condition
 * as a 00 unconditional jump with the C flag set (so it's really a conditional
 * jump), and then the additional condition codes don't have to be checked?
 * 
 * Hmm. That might not actually be that useful. In fact, we would probably prefer
 * the C flag to ONLY be set explicitly. That way, when we want conditional execution,
 * we can do something like put multiple C instructions in a row, and then either
 * execute ALL of them or NONE of them.
 * 
 * Maybe ALL instructions are conditional?
 * [C: 1] [I_TYPE: 2]
 * X00: 0 argument:
 *    ~16 different instructions? 4 bits for instruction type. 
 *    0000: singleton instructions. Other 25 bits choose the instruction. 
 *        - halt
 *        - nop
 *        - syscall
 *        - return
 *        - return_if_null
 *    0001: unconditional jump relative to pc.
 *    0010: unconditional jump-and-link relative to pc.
 * X01: 1 argument
 *    VV: vec
 *      ZZ: size
 *        AAAAA: The register
 *             CCCC: The instruction
 *                 iiii iiii iiii iiii: immediate value
 * 1-op instructions we want:
 * - load immediate
 * - load upper immediate
 * - load upper upper immediate
 * - load upper upper upper immediate
 * - add with immediate
 * - sub with immediate
 * - 
 *             
 * X10: 2 argument
 *    VV: vec
 *      ZZ: size
 *        AAAAA: Dest
 *             BBBBB: Src
 *                  IIIII IIIII IIIII: Instruction
 * 2-op instructions we want:
 * - multiply
 * - divide
 * - normalize
 * - length
 * - dot product?
 * - cross product?
 * - comparison? 
 * - swizzle
 * - format conversion
 * - bitwise not
 * - bitwise shifts
 * - negate
 * - logical not?
 * 
 *
 * X11: 3 argument
 *    VV: vec
 *      ZZ: size
 *        AAAAA: dest
 *             BBBBB: src1
 *                  CCCCC: src2
 *                       IIIII IIIII: Instruction (1024 options?)
 * 3-op instructions we want:
 * - add
 * - sub
 * - min (note: needs float/signed/unsigned disctinction)
 * - max
 * - min-of-components
 * - max-of-components
 * - bitwise and
 * - bitwise or
 * - // this shuld be two arg: bitwise not
 * - logical and?
 * - logical or?
 * - load/store:
 *   - many flags.
 *   - ld a0, [a1 + 4 * a2 + 4]
 *   - so we have:
 *     - multiply value for second arg (?)
 *     - constant offset val
 *     - some masking operations:
 *       - is the src1 pointer masked?
 *       - is the src2 offset masked?
 *       - the expression result is always masked (no unaligned load/stores)
 *     - is the load/store atomic? (?)
 */



struct favor_op_info {
    const char *name;

    uint32_t opcode;
    
    union { int32_t funct_u; uint32_t funct_cc; uint32_t funct_j; uint32_t funct; };
    int32_t funct_s;
    int32_t funct_f;
    int32_t funct_x;
};

// struct favor_ty_info {
//     const char *name;

//     uint32_t u: 1;
//     uint32_t f: 1;
//     uint32_t sz: 2;
//     uint32_t vec: 2;
// };

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

FAVOR_OP_TABLE_DECLARE(ld_imm)

extern struct favor_op_info favor_op_table[];
extern size_t favor_op_table_size;

extern struct favor_reg_info favor_reg_table[];
extern size_t favor_reg_table_size;

// extern struct favor_ty_info favor_ty_table[];
// extern size_t favor_ty_table_size;

/* Instruction kind: 0 arg, 1 arg, 2 arg, 3 arg */
enum opcode {
    OP_CC_MISC,
    OP_JUMP,
    OP_INT3,
    OP_INT2,
    OP_FLOAT3,
    OP_FLOAT2,
    OP_FIX2,
    OP_LOAD,
    OP_STORE,
    OP_LS_SPECIAL,

    /* Psuedo-opcodes:
     * These do not fit in the actual 4-bit opcode field of the instruction.
     * Instead, they indicate sub-types of the struct insn to make working
     * with various opcodes easier, such as ld_imm opcodes. */
    POP_START = 16,
    POP_LD_IMM,
};

/**
 * Not real opcodes, but or'd in with the opcode field in favor_op_info to
 * help narrow down certain things.
 */
enum opcode_flag {
    OP_CC_MISC_SINGLETON = 0x100 | OP_CC_MISC,
    OP_INT_FLOAT_3 = 0x200,
};

/**
 * Conditions--ways to compute a single boolean result based on the status flags.
 * Always encoded in the same order (and ideally in the same bit position).
 */
enum condition {
    COND_EQ,
    COND_NE,
    COND_G,
    COND_L,
    COND_GE,
    COND_LE,
    COND_GU,
    COND_LU,
    COND_GEU,
    COND_LEU,
    COND_NEG,
    // really non-negative: todo better name?
    COND_POS
};

enum cc_misc {
    CCM_ILL = 0,
    CCM_NOP,
    CCM_HALT,
    CCM_SYSCALL,
    CCM_SETEQ,
    CCM_SETNE,
    CCM_SETG,
    CCM_SETL,
    CCM_SETGE,
    CCM_SETLE,
    CCM_SETGU,
    CCM_SETLU,
    CCM_SETGEU,
    CCM_SETLEU,
    CCM_SETNEG,
    CCM_SETPOS,
    CCM_TEST, /* Test the dest register, set C = 1 if the bit is set */
    CCM_READC,
    CCM_WRITEC,
    CCM_WRITEC_IMM,
};

enum jump {
    J_JUMP,
    J_BEQ,
    J_BNE,
    J_BL,
    J_BG,
    J_BLE,
    J_BGE,
    J_BLU,
    J_BGU,
    J_BLEU,
    J_BGEU,

    JC_JUMP,
    JC_BEQ,
    JC_BNE,
    JC_BL,
    JC_BG,
    JC_BLE,
    JC_BGE,
    JC_BLU,
    JC_BGU,
    JC_BLEU,
    JC_BGEU,

    JIC_JUMP,
    JIC_BEQ,
    JIC_BNE,
    JIC_BL,
    JIC_BG,
    JIC_BLE,
    JIC_BGE,
    JIC_BLU,
    JIC_BGU,
    JIC_BLEU,
    JIC_BGEU,
};

enum int3 {
    I3_ADD,
    I3_SUB,
    I3_LSH,
    I3_RSHU,
    I3_RSHS,
    I3_ROL,
    I3_ROR,
    I3_AND,
    I3_OR,
    I3_XOR,
    I3_MINS,
    I3_MINU,
    I3_MAXS,
    I3_MAXU,
    I3_LOG_AND,
    I3_LOG_OR,
    I3_SAT_ADDS,
    I3_SAT_ADDU,
    I3_SAT_SUBS,
    I3_SAT_SUBU
};

enum int2 {
    I2_NEGATE,
    I2_NOT,
    I2_LOG_NOT,
    I2_LOG_IDENTITY,
    I2_CMP,
    I2_SWIZZLE, /* Requires 8-bit arg */
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

enum ld_imm {
    LDI3U,
    LDI3O,

    LDI2S,
    LDI2U,
    LDI2O,

    LDI1S,
    LDI1U,
    LDI1O,

    LDI0S,
    LDI0U,
    LDI0O,
    LDI0OPC,
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
    uint32_t p_opcode : 6;

    union {
        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t shift : 6;
            uint32_t is_return : 1;
            uint32_t vec : 2;
            uint32_t funct : 13;
        } cc_misc;

        struct {
            uint32_t funct : 5;
            uint32_t and_link : 1;
            uint32_t immediate : 22;
        } jump;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t src2 : 5;
            uint32_t sz : 2;
            uint32_t vec : 2;
            uint32_t funct : 8; 
        } int3;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t sz : 2;
            uint32_t vec : 2;
            uint32_t funct : 13;
        } int2;

        struct {
            uint32_t conditional: 1;
            uint32_t dest: 5;
            uint32_t sz: 2;
            uint32_t funct: 4;
            uint32_t imm: 16;
        } int_imm;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t src2 : 5;
            uint32_t sz : 1;
            uint32_t vec : 2;
            uint32_t funct : 9; 
        } float3;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t sz : 1;
            uint32_t vec : 2;
            uint32_t funct : 14;
        } float2;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t sz : 2;
            uint32_t vec : 2;
            uint32_t funct : 13;
        } fix2;

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t src2 : 5;
            uint32_t sz : 2;
            uint32_t vec : 2;
            uint32_t fp : 1;
            uint32_t offset : 5;
            uint32_t shift : 2;
        } ls; /* load-store */

        /* struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t vec : 2;
            uint32_t fp : 1;
            uint32_t etc : 14;
        } ls_special; */ /* COME BACK TO THIS */

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t imm: 16;
            uint32_t fp : 1;
            uint32_t unused : 1;
            uint32_t funct: 4;
        } ld_imm;
    };
};

static inline
struct insn
mk_basic_cc_misc(uint32_t conditional, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = OP_CC_MISC;
    result.cc_misc.conditional = conditional;
    result.cc_misc.funct = funct;
    return result;
}

static inline
struct insn
mk_int3(uint32_t conditional, uint32_t dest, uint32_t src1, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = OP_INT3;
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
mk_float3(uint32_t conditional, uint32_t dest, uint32_t src1, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.p_opcode = OP_FLOAT3;
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