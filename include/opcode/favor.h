#ifndef OPCODE_FAVOR_H
#define OPCODE_FAVOR_H

/* opcodes for the FAVOR isa. */

#include <stdint.h>

/* New(er) instruction type ideas:
 * - 0 argument (halt, nop, unconditional jump, conditional jump)
 * - 1 argument (add immediate)
 * - 2 argument (swizzle) 
 * - 3 argument (add a, b, c) 
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
 *    0001: unconditional jump relative to pc.
 * X01: 1 argument
 *    VV: vec
 *      ZZ: size
 *        AAAAA: The register
 *             
 *    
 *   */

/** Whether the instruction is a fixed point instruction */
#define FAVOR_FLAG_X 0x80000000
/** Whether the instruction is a regular register instruction */
#define FAVOR_FLAG_R 0X40000000

#define FAVOR_IISN_LDI 0
#define FAVOR_IISN_JMP 2
#define FAVOR_IISN_LDH 3
#define FAVOR_IISN_ADD 4
/**
 * Load and jump: Loads constant values based on the sz * vec,
 * then jumps over that much data. */
#define FAVOR_IISN_LDJ 5

#define FAVOR_RISN_HLT 0
#define FAVOR_RISN_ADD 1
/* Store <dst> at <op1> + <op2> * sizeof(op) * 1 */
#define FAVOR_RISN_ST1 2
/* Load <dst> from <op1> + <op2> * sizeof(op) * 1 */
#define FAVOR_RISN_LD1 3

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

static inline
uint32_t
favor_assemble_i(uint32_t iisn, uint32_t imm, uint32_t reg, uint32_t sz, uint32_t vec, uint32_t c) {
    c    = c    & 0x01;
    vec  = vec  & 0x03;
    sz   = sz   & 0x03;
    reg  = reg  & 0x1F;
    iisn = iisn & 0x1F;
    imm  = imm  & 0xFFFF;
    return (c << 0) | (vec << 1) | (sz << 3) | (reg << 5) | (imm << 10) | (iisn << 26);
}

static inline
uint32_t
favor_assemble_r(uint32_t risn, uint32_t u, uint32_t dst, uint32_t op1, uint32_t op2, uint32_t sz, uint32_t vec, uint32_t c) {
    risn = risn & 0xFF;
    u    = u    & 0x01;
    dst  = dst  & 0x1F;
    op1  = op1  & 0x1F;
    op2  = op2  & 0x1F;
    sz   = sz   & 0x03;
    vec  = vec  & 0x03;
    c    = c    & 0x01;
    return
        FAVOR_FLAG_R |
        (c    <<  0) |
        (vec  <<  1) |
        (sz   <<  3) |
        (op2  <<  5) |
        (op1  << 10) |
        (dst  << 15) |
        (u    << 20) |
        (risn << 21);
}

#endif