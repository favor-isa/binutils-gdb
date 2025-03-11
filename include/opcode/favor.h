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


/* Instruction kind: 0 arg, 1 arg, 2 arg, 3 arg */
#define FAVOR_K0       0
#define FAVOR_K1       1
#define FAVOR_K2       2
#define FAVOR_K3       3

enum {
    OP_K0_SINGLETON = 0,

    OP_SNG_HALT = 0,
    OP_SNG_SYSCALL = 1,
};

#define FAVOR_HALT     0

#define FAVOR_REG_ZERO 0
#define FAVOR_REG_A0   8
#define FAVOR_REG_A1   9
#define FAVOR_REG_A2   10
#define FAVOR_REG_A3   11
#define FAVOR_REG_A4   12
#define FAVOR_REG_A5   13
#define FAVOR_REG_A6   14
#define FAVOR_REG_A7   15

#define FAVOR_SZ_BYTE 0
#define FAVOR_SZ_HALF 1
#define FAVOR_SZ_WORD 2
#define FAVOR_SZ_LONG 3

#define FAVOR_VEC1 0
#define FAVOR_VEC2 1
#define FAVOR_VEC3 2
#define FAVOR_VEC4 3

#define FAVOR_C_UNCONDITIONAL 0
#define FAVOR_C_CONDITIONAL   1

/**
 * Helper struct for easily encoding / decoding instructions.
 */
struct favor_insn {
    uint32_t c :    1;
    uint32_t kind : 2;
    union {
        struct {
            uint32_t k0_code : 4;
            uint32_t k0_imm  : 25;
        };

        struct {
            uint32_t vec  : 2;
            uint32_t sz   : 2;
            uint32_t reg0 : 5;
            union {
                uint32_t k1_code : 4;
                uint32_t k1_imm  : 16;
            };
        };
    };
};

static inline
struct favor_insn
favor_k0_insn(uint32_t c, uint32_t op, uint32_t imm) {
    struct favor_insn insn;
    insn.c = c;
    insn.kind = FAVOR_K0;
    insn.k0_code = op;
    insn.k0_imm = imm;
    return insn;
}

static inline
struct favor_insn
favor_singleton(uint32_t c, uint32_t op) {
    return favor_k0_insn(c, OP_K0_SINGLETON, op);
}

static inline
uint32_t
favor_encode(struct favor_insn insn) {
    uint32_t value = 0;
    value |= (insn.c    << 31);
    value |= (insn.kind << 29);

    // All other ones have a vec, sz, reg0
    if(insn.kind != FAVOR_K0) {
        insn.vec |= (insn.vec << 27);
        insn.sz  |= (insn.sz  << 25);
    }

    switch(insn.kind) {
        case FAVOR_K0:
            value |= (insn.k0_code << 25);
            value |= (insn.k0_imm  << 0);
            break;
        case FAVOR_K1:
            value |= (insn.k1_code << 16);
            value |= (insn.k1_imm  << 0);
            break;
        
    }

    return value;
}

static inline
struct favor_insn
favor_decode(uint32_t code) {
    struct favor_insn insn;

    // Do bitfields automatically get masked out? Convenient if true.
    insn.c    = (code >> 31);
    insn.kind = (code >> 29);

    if(insn.kind != FAVOR_K0) {
        insn.vec = (code >> 27);
        insn.sz  = (code >> 25);
    }

    switch(insn.kind) {
        case FAVOR_K0:
            insn.k0_code = (code >> 25);
            insn.k0_imm  = (code >>  0);
            break;
        case FAVOR_K1:
            insn.k1_code = (code >> 16);
            insn.k1_imm  = (code >>  0);
            break;
    }

    return insn;
}

#endif