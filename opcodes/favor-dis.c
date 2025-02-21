#include "sysdep.h"
#include <stdio.h>

#include "disassemble.h"

// TODO: Fgure out if we want these options.
#define STATIC_TABLE
#define DEFINE_TABLE

#include "opcode/favor.h"
#include "dis-asm.h"

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
    struct favor_insn insn = favor_decode(op);

    switch(insn.kind) {
        case FAVOR_K0:
            switch(insn.k0_code) {
                case FAVOR_HALT:
                    pr(stream, "halt");
                    break;
                default:
                    pr(stream, "(bad k0)");
                    break;
            };
            break;
        case FAVOR_K1:
            switch(insn.k1_code) {
                default:
                    pr(stream, "(bad k1)");
                    break;
            };
            break;
        case FAVOR_K2:
            pr(stream, "(bad k2)");
            break;
        case FAVOR_K3:
            pr(stream, "(bad k3)");
            break;
    }

    return 4; // The number of bytes to advance. -1 on error?
}