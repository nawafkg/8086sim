#include "decoder.h"

static const char *reg8[8]  = { "al","cl","dl","bl","ah","ch","dh","bh" };
static const char *reg16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };
static const char *ea_table[8] = {
    "bx + si", // r/m = 000
    "bx + di", // r/m = 001
    "bp + si", // r/m = 010
    "bp + di", // r/m = 011
    "si",      // r/m = 100
    "di",      // r/m = 101
    "bp",      // r/m = 110  (EXCEPT when mod == 00)
    "bx"       // r/m = 111
};

void handle_mov_rm_r(unsigned char first, FILE *in, FILE *out) {

    unsigned char d = (first >> 1) & 1;
    unsigned char w = first & 1;

    int modrm_i = fgetc(in);
    if (modrm_i == EOF) {
        fprintf(stderr, "Unexpected EOF in MOV\n");
        return;
    }

    unsigned char modrm = (unsigned char)modrm_i;
    unsigned char mod = (modrm >> 6) & 0b11;
    unsigned char reg = (modrm >> 3) & 0b111;
    unsigned char rm  = modrm & 0b111;

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
    
    case 0b00: //memory - reg but no displacement
        switch (d) {
        case 0b0:
            //mov r/m, reg
            fprintf(out, "mov [%s], %s", ea_table[rm], regs[reg]);
            break;
        case 0b1:
            //mov reg, r/m
            //special case if rm = 110
            if (rm == 0b110) fprintf(out, "mov %s, [disp16]", regs[reg]);
            else fprintf(out, "mov %s, [%s]", regs[reg], ea_table[rm]);
            break;
        }
        break;

    case 0b01: { // memory - reg + 8-bit displacement
        int disp_i = fgetc(in);
        if (disp_i == EOF) {
            fprintf(stderr, "Unexpected EOF in MOV disp8\n");
            return;
        }

        signed char disp8 = (signed char)disp_i;

        switch (d) {
        case 0:
            // mov r/m, reg
            fprintf(out, "mov [%s %c %d], %s",
                    ea_table[rm],
                    disp8 >= 0 ? '+' : '-',
                    disp8 >= 0 ? disp8 : -disp8,
                    regs[reg]);
            break;

        case 1:
            // mov reg, r/m
            fprintf(out, "mov %s, [%s %c %d]",
                    regs[reg],
                    ea_table[rm],
                    disp8 >= 0 ? '+' : '-',
                    disp8 >= 0 ? disp8 : -disp8);
            break;
        }
        break;
    }

    case 0b10: { // memory - reg + 16-bit displacement
        int lo = fgetc(in);
        int hi = fgetc(in);
        if (lo == EOF || hi == EOF) {
            fprintf(stderr, "Unexpected EOF in MOV disp16\n");
            return;
        }

        signed short disp16 = (signed short)(lo | (hi << 8));

        switch (d) {
        case 0:
            // mov r/m, reg
            fprintf(out, "mov [%s %c %d], %s",
                    ea_table[rm],
                    disp16 >= 0 ? '+' : '-',
                    disp16 >= 0 ? disp16 : -disp16,
                    regs[reg]);
            break;

        case 1:
            // mov reg, r/m
            fprintf(out, "mov %s, [%s %c %d]",
                    regs[reg],
                    ea_table[rm],
                    disp16 >= 0 ? '+' : '-',
                    disp16 >= 0 ? disp16 : -disp16);
            break;
        }
        break;
    }

    default:
        fprintf(out, "mov ; not implemented");
        break;
    }

}

void handle_mov_imm_r(unsigned char first, FILE *in, FILE *out) {

    unsigned char w = (first >> 3) & 1;
    unsigned char reg = first & 0b111;

    switch (w) {
    case 0b0: {
        int imm_int = fgetc(in);
        if (imm_int == EOF) {
            fprintf(stderr, "Unexpected EOF in MOV\n");
            return;
        }

        unsigned char imm = (unsigned char)imm_int;
        fprintf(out, "mov %s, %d", reg8[reg], imm);
        break;
    }
        
    
    case 0b1: {
        int lo = fgetc(in);
        int hi = fgetc(in);

        if (lo == EOF || hi == EOF) {
            fprintf(stderr, "Unexpected EOF in MOV\n");
            return;
        }
        signed short imm = (signed short)((hi << 8) | lo);
        fprintf(out, "mov %s, %d", reg16[reg], imm);
        break;
    }
    }

}

void decode_file(FILE *in, FILE *out)
{
    int byte;

    while ((byte = fgetc(in)) != EOF) {
        unsigned char first = (unsigned char)byte;
        // unsigned char opcode = first >> 2;

        if((first & 0b11111100) == 0b10001000) {
            handle_mov_rm_r(first, in, out);
        }

        else if ((first & 0b11110000) == 0b10110000) {
            handle_mov_imm_r(first, in, out);
        }

        else {
            fprintf(stderr, "Unsupported instruction: 0x%02X\n", first);
            return;
        }

        // switch (opcode) {
        // case 0b100010:  // MOV
        //     handle_mov(first, in, out);
        //     break;

        // default:
        //     fprintf(stderr, "Unsupported instruction: 0x%02X\n", first);
        //     return;
        // }

        fputc('\n', out);
    }
}



