#include "decoder.h"

static const char *reg8[8]  = { "al","cl","dl","bl","ah","ch","dh","bh" };
static const char *reg16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };

void handle_mov(unsigned char first, FILE *in, FILE *out) {

    unsigned char d = (first >> 1) & 1;
    unsigned char w = first & 1;

    int modrm_i = fgetc(in);
    if (modrm_i == EOF) {
        fprintf(stderr, "Unexpected EOF in MOV\n");
        return;
    }

    unsigned char modrm = (unsigned char)modrm_i;
    unsigned char mod = (modrm >> 6) & 0x3;
    unsigned char reg = (modrm >> 3) & 0x7;
    unsigned char rm  = modrm & 0x7;

    const char **regs = w ? reg16 : reg8;

    switch (mod) {
    case 0b11:  // register ↔ register
        switch (d) {
        case 0:
            // mov r/m, reg
            fprintf(out, "mov %s, %s", regs[rm], regs[reg]);
            break;
        case 1:
            // mov reg, r/m
            fprintf(out, "mov %s, %s", regs[reg], regs[rm]);
            break;
        }
        break;

    default:
        fprintf(out, "mov ; not implemented");
        break;
    }

}

void decode_file(FILE *in, FILE *out)
{
    int byte;

    while ((byte = fgetc(in)) != EOF) {
        unsigned char first = (unsigned char)byte;
        unsigned char opcode = first >> 2;

        switch (opcode) {
        case 0b100010:  // MOV
            handle_mov(first, in, out);
            break;

        default:
            fprintf(stderr, "Unsupported instruction: 0x%02X\n", first);
            return;
        }

        fputc('\n', out);
    }
}



