#include "sysdep.h"
#include "opcode/favor.h"

#define UF(uv, fv)      .funct_u = uv, .funct_s = -1, .funct_f = fv, .funct_x = -1
#define U(uv)           .funct_u = uv, .funct_s = -1, .funct_f = -1, .funct_x = -1
#define US(uv, sv)      .funct_u = uv, .funct_s = sv, .funct_f = -1, .funct_x = -1
#define USF(uv, sv, fv) .funct_u = uv, .funct_s = sv, .funct_f = fv, .funct_x = -1

/* Table mapping opcode names to data. */
struct favor_op_info
favor_op_table[] = {
    { .name = "add", .opcode = OP_INT3, UF(I3_ADD, F3_ADD) },
    { .name = "sub", .opcode = OP_INT3, UF(I3_SUB, F3_SUB) },
    { .name = "lsh", .opcode = OP_INT3, U (I3_LSH) },
    { .name = "rsh", .opcode = OP_INT3, US(I3_RSHU, I3_RSHS) },
    { .name = "rol", .opcode = OP_INT3, U (I3_ROL) },
    { .name = "ror", .opcode = OP_INT3, U (I3_ROR) },
    { .name = "and", .opcode = OP_INT3, U (I3_AND) },
    { .name = "or" , .opcode = OP_INT3, U (I3_OR) },
    { .name = "xor", .opcode = OP_INT3, U (I3_XOR) },
    { .name = "min", .opcode = OP_INT3, USF(I3_MINU, I3_MINS, F3_MIN) },
    { .name = "max", .opcode = OP_INT3, USF(I3_MAXU, I3_MAXS, F3_MAX) },
    { .name = "logand", .opcode = OP_INT3, U(I3_LOG_AND) },
    { .name = "logor" , .opcode = OP_INT3, U(I3_LOG_OR) },
    { .name = "satadd", .opcode = OP_INT3, US(I3_SAT_ADDU, I3_SAT_ADDS) },
    { .name = "satsub", .opcode = OP_INT3, US(I3_SAT_ADDU, I3_SAT_ADDS) },


    { .name = "ill",     .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_ILL },
    { .name = "nop",     .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_NOP },
    { .name = "halt",    .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_HALT },
    { .name = "syscall", .opcode = OP_CC_MISC_SINGLETON, .funct_cc = CCM_SYSCALL },
};

size_t favor_op_table_size = sizeof(favor_op_table) / sizeof(*favor_op_table);