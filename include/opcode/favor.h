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
    
    union { int32_t funct_u; uint32_t funct_cc; };
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
};

/**
 * Not real opcodes, but or'd in with the opcode field in favor_op_info to
 * help narrow down certain things.
 */
enum opcode_flag {
    OP_CC_MISC_SINGLETON = 0x100 | OP_CC_MISC,
    OP_INT_FLOAT_3 = 0x200,
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

enum gpr {
    REG_A0,
    REG_A1,
    REG_A2,
    REG_A3,
    REG_A4,
    REG_A5,
    REG_A6,
    REG_A7,
    REG_V0,
    REG_V1,
    REG_V2,
    REG_V3,
    REG_V4,
    REG_V5,
    REG_V6,
    REG_V7,
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

    REG_SP,
    REG_FP,
    REG_LA,
    REG_ZERO
};

/* TODO:
 * Consider combining int3 + int2 into one chunk, and
 * float3 + float2 into one chunk. */

/**
 * Helper struct for easily encoding / decoding instructions.
 */
struct insn {
    uint32_t opcode : 4;
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
            uint32_t immediate : 23;
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

        struct {
            uint32_t conditional : 1;
            uint32_t dest : 5;
            uint32_t src1 : 5;
            uint32_t vec : 2;
            uint32_t fp : 1;
            uint32_t etc : 19;
        } ls_special;
    };
};

static inline
struct insn
mk_basic_cc_misc(uint32_t conditional, uint32_t funct) {
    struct insn result = {0};
    result.opcode = OP_CC_MISC;
    result.cc_misc.conditional = conditional;
    result.cc_misc.funct = funct;
    return result;
}

static inline
struct insn
mk_int3(uint32_t conditional, uint32_t dest, uint32_t src1, uint32_t src2, uint32_t sz, uint32_t vec, uint32_t funct) {
    struct insn result = {0};
    result.opcode = OP_INT3;
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
    result.opcode = OP_FLOAT3;
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
uint32_t
favor_encode(struct insn insn) {
    uint32_t value = 0;
    value |= (insn.opcode);

    switch(insn.opcode) {
        case OP_CC_MISC:
            value |= (insn.cc_misc.conditional << 4);
            value |= (insn.cc_misc.dest        << 5);
            value |= (insn.cc_misc.shift       << 10);
            value |= (insn.cc_misc.is_return   << 16);
            value |= (insn.cc_misc.vec         << 17);
            value |= (insn.cc_misc.funct       << 19);
            break;
        case OP_JUMP:
            value |= (insn.jump.funct     << 4);
            value |= (insn.jump.and_link  << 9);
            value |= (insn.jump.immediate << 10);
            break;
        case OP_INT3:
            value |= (insn.int3.conditional << 4);
            value |= (insn.int3.dest        << 5);
            value |= (insn.int3.src1        << 10);
            value |= (insn.int3.src2        << 15);
            value |= (insn.int3.sz          << 20);
            value |= (insn.int3.vec         << 22);
            value |= (insn.int3.funct       << 24);
            break;
        case OP_INT2: {
            uint32_t funct = insn.int2.funct;
            value |= (insn.int2.conditional << 4);
            value |= (insn.int2.dest        << 5);
            value |= (insn.int2.src1        << 10);
            value |= ((funct & 0x1F)        << 15);
            funct >>= 5;
            value |= (insn.int2.sz          << 20);
            value |= (insn.int2.vec         << 22);
            value |= (funct                 << 24);
            break;
        }
        case OP_LOAD:
        case OP_STORE: {
            value |= (insn.ls.conditional << 4);
            value |= (insn.ls.dest        << 5);
            value |= (insn.ls.src1        << 10);
            value |= (insn.ls.src2        << 15);
            value |= (insn.ls.sz          << 20);
            value |= (insn.ls.vec         << 22);
            value |= (insn.ls.fp          << 24);
            value |= (insn.ls.offset      << 25);
            value |= (insn.ls.shift       << 30);
            break;
        }
    }

    return value;
}

static inline
struct insn
favor_decode(uint32_t code) {
    struct insn insn;

    // Do bitfields automatically get masked out? Convenient if true.
    insn.opcode = code;

    switch(insn.opcode) {
        case OP_CC_MISC:
            insn.cc_misc.conditional = code >> 4;
            insn.cc_misc.dest        = code >> 5;
            insn.cc_misc.shift       = code >> 10;
            insn.cc_misc.is_return   = code >> 16;
            insn.cc_misc.vec         = code >> 17;
            insn.cc_misc.funct       = code >> 19;
            break;
        case OP_JUMP:
            insn.jump.funct     = code >> 4 ;
            insn.jump.and_link  = code >> 9 ;
            insn.jump.immediate = code >> 10;
            break;
        case OP_INT3:
            insn.int3.conditional = code >> 4 ;
            insn.int3.dest        = code >> 5 ;
            insn.int3.src1        = code >> 10;
            insn.int3.src2        = code >> 15;
            insn.int3.sz          = code >> 20;
            insn.int3.vec         = code >> 22;
            insn.int3.funct       = code >> 24;
            break;
        case OP_INT2: {
            uint32_t funct = insn.int2.funct;
            insn.int2.conditional = code >> 4;
            insn.int2.dest        = code >> 5;
            insn.int2.src1        = code >> 10;
            funct                |= (code >> 15) & 0x1F;
            funct <<= 5;
            insn.int2.sz          = code >> 20;
            insn.int2.vec         = code >> 22;
            funct                |= code >> 24;
            break;
        }
        case OP_LOAD:
        case OP_STORE: {
            insn.ls.conditional = code >> 4 ;
            insn.ls.dest        = code >> 5 ;
            insn.ls.src1        = code >> 10;
            insn.ls.src2        = code >> 15;
            insn.ls.sz          = code >> 20;
            insn.ls.vec         = code >> 22;
            insn.ls.fp          = code >> 24;
            insn.ls.offset      = code >> 25;
            insn.ls.shift       = code >> 30;
            break;
        }
    }

    return insn;
}

#endif