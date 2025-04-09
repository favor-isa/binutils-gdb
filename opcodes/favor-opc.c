#include "sysdep.h"
#include "opcode/favor.h"

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
};

size_t favor_op_table_size = sizeof(favor_op_table) / sizeof(*favor_op_table);

#define REG(r) .reg = r, .reg_mask = (r & 0x1F), .f = (r >> 5)

struct favor_reg_info favor_reg_table[] = {
    { .name = "a0", REG(REG_A0) },
    { .name = "a1", REG(REG_A1) },
    { .name = "a2", REG(REG_A2) },
    { .name = "a3", REG(REG_A3) },
    { .name = "a4", REG(REG_A4) },
    { .name = "a5", REG(REG_A5) },
    { .name = "a6", REG(REG_A6) },
    { .name = "a7", REG(REG_A7) },
    { .name = "v0", REG(REG_V0) },
    { .name = "v1", REG(REG_V1) },
    { .name = "v2", REG(REG_V2) },
    { .name = "v3", REG(REG_V3) },
    { .name = "v4", REG(REG_V4) },
    { .name = "v5", REG(REG_V5) },
    { .name = "v6", REG(REG_V6) },
    { .name = "v7", REG(REG_V7) },
    { .name = "t0", REG(REG_T0) },
    { .name = "t1", REG(REG_T1) },
    { .name = "t2", REG(REG_T2) },
    { .name = "t3", REG(REG_T3) },
    { .name = "t4", REG(REG_T4) },
    { .name = "t5", REG(REG_T5) },
    { .name = "t6", REG(REG_T6) },
    { .name = "t7", REG(REG_T7) },
    { .name = "t8", REG(REG_T8) },
    { .name = "t9", REG(REG_T9) },
    { .name = "t10", REG(REG_T10) },
    { .name = "t11", REG(REG_T11) },
    { .name = "sp", REG(REG_SP) },
    { .name = "fp", REG(REG_FP) },
    { .name = "la", REG(REG_LA) },
    { .name = "zero", REG(REG_ZERO) },
};
size_t favor_reg_table_size = sizeof(favor_reg_table) / sizeof(*favor_reg_table);