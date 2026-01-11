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

static const char *jcc_table[16] = {
    "jo",   "jno",  "jb",   "jnb",
    "je",   "jne",  "jbe",  "ja",
    "js",   "jns",  "jp",   "jnp",
    "jl",   "jnl",  "jle",  "jg"
};

void handle_jcc(unsigned char first, FILE *in, FILE *out)
{
    int b = fgetc(in);
    if (b == EOF) { fprintf(stderr, "Unexpected EOF in Jcc\n"); return; }

    signed char off = (signed char)b;
    unsigned char cc = first & 0b00001111;

    fprintf(out, "%s %d", jcc_table[cc], (int)off);
}

void handle_loop_family(unsigned char first, FILE *in, FILE *out)
{
    int b = fgetc(in);
    if (b == EOF) { fprintf(stderr, "Unexpected EOF in loop/jcxz\n"); return; }

    signed char off = (signed char)b;

    switch (first) {
    case 0b11100010: fprintf(out, "loop %d",   (int)off); break;
    case 0b11100001: fprintf(out, "loopz %d",  (int)off); break;
    case 0b11100000: fprintf(out, "loopnz %d", (int)off); break;
    case 0b11100011: fprintf(out, "jcxz %d",   (int)off); break;
    }
}

static const char *alu_imm_op(unsigned char reg) {
    switch (reg) {
    case 0b000: return "add";
    case 0b101: return "sub";
    case 0b111: return "cmp";
    default:    return NULL;
    }
}

static const char *alu_acc_mnemonic(unsigned char first) {
    switch (first) {
    case 0b00000100: case 0b00000101: return "add"; // 0000010w
    case 0b00101100: case 0b00101101: return "sub"; // 0010110w
    case 0b00111100: case 0b00111101: return "cmp"; // 0011110w
    default: return NULL;
    }
}

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
    
    case 0b00:
        switch (d) {
        case 0: // r/m, reg
            if (rm == 0b110) {
                int lo = fgetc(in);
                int hi = fgetc(in);
                if (lo == EOF || hi == EOF) {
                    fprintf(stderr, "Unexpected EOF in disp16\n");
                    return;
                }
                unsigned short disp = (unsigned short)(lo | (hi << 8));
                fprintf(out, "mov [%u], %s", disp, regs[reg]);
            } else {
                fprintf(out, "mov [%s], %s", ea_table[rm], regs[reg]);
            }
            break;

        case 1: // reg, r/m
            if (rm == 0b110) {
                int lo = fgetc(in);
                int hi = fgetc(in);
                if (lo == EOF || hi == EOF) {
                    fprintf(stderr, "Unexpected EOF in disp16\n");
                    return;
                }
                unsigned short disp = (unsigned short)(lo | (hi << 8));
                fprintf(out, "mov %s, [%u]", regs[reg], disp);
            } else {
                fprintf(out, "mov %s, [%s]", regs[reg], ea_table[rm]);
            }
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

void handle_alu_rm_r(const char *mnemonic, unsigned char first, FILE *in, FILE *out) {
    unsigned char d = (first >> 1) & 1;
    unsigned char w = first & 1;

    int modrm_i = fgetc(in);
    if (modrm_i == EOF) {
        fprintf(stderr, "Unexpected EOF in %s\n", mnemonic);
        return;
    }

    unsigned char modrm = (unsigned char)modrm_i;
    unsigned char mod = (modrm >> 6) & 3;
    unsigned char reg = (modrm >> 3) & 7;
    unsigned char rm  = modrm & 7;

    const char **regs = w ? reg16 : reg8;

    switch (mod) {

    case 3: {  // register ↔ register
        if (d == 0)
            fprintf(out, "%s %s, %s", mnemonic, regs[rm], regs[reg]);
        else
            fprintf(out, "%s %s, %s", mnemonic, regs[reg], regs[rm]);
        break;
    }

    case 0: {  // memory, no displacement
        if (rm == 6) {
            int lo = fgetc(in);
            int hi = fgetc(in);
            if (lo == EOF || hi == EOF) {
                fprintf(stderr, "Unexpected EOF in disp16\n");
                return;
            }
            unsigned short disp = (unsigned short)(lo | (hi << 8));

            if (d == 0)
                fprintf(out, "%s [%u], %s", mnemonic, disp, regs[reg]);
            else
                fprintf(out, "%s %s, [%u]", mnemonic, regs[reg], disp);
        } else {
            if (d == 0)
                fprintf(out, "%s [%s], %s", mnemonic, ea_table[rm], regs[reg]);
            else
                fprintf(out, "%s %s, [%s]", mnemonic, regs[reg], ea_table[rm]);
        }
        break;
    }

    case 1: {  // disp8
        int disp_i = fgetc(in);
        if (disp_i == EOF) {
            fprintf(stderr, "Unexpected EOF in disp8\n");
            return;
        }

        signed char disp8 = (signed char)disp_i;

        if (d == 0)
            fprintf(out, "%s [%s %c %d], %s",
                    mnemonic, ea_table[rm],
                    disp8 >= 0 ? '+' : '-',
                    disp8 >= 0 ? disp8 : -disp8,
                    regs[reg]);
        else
            fprintf(out, "%s %s, [%s %c %d]",
                    mnemonic, regs[reg],
                    ea_table[rm],
                    disp8 >= 0 ? '+' : '-',
                    disp8 >= 0 ? disp8 : -disp8);
        break;
    }

    case 2: {  // disp16
        int lo = fgetc(in);
        int hi = fgetc(in);
        if (lo == EOF || hi == EOF) {
            fprintf(stderr, "Unexpected EOF in disp16\n");
            return;
        }

        signed short disp16 = (signed short)(lo | (hi << 8));

        if (d == 0)
            fprintf(out, "%s [%s %c %d], %s",
                    mnemonic, ea_table[rm],
                    disp16 >= 0 ? '+' : '-',
                    disp16 >= 0 ? disp16 : -disp16,
                    regs[reg]);
        else
            fprintf(out, "%s %s, [%s %c %d]",
                    mnemonic, regs[reg],
                    ea_table[rm],
                    disp16 >= 0 ? '+' : '-',
                    disp16 >= 0 ? disp16 : -disp16);
        break;
    }

    default:
        fprintf(out, "%s ; not implemented", mnemonic);
        break;
    }
}

void handle_alu_imm_rm(unsigned char first, FILE *in, FILE *out) {
    unsigned char s = (first >> 1) & 1;   // sign-extend imm8 if w=1
    unsigned char w = first & 1;          // 0=byte, 1=word

    int modrm_i = fgetc(in);
    if (modrm_i == EOF) {
        fprintf(stderr, "Unexpected EOF in ALU imm\n");
        return;
    }

    unsigned char modrm = (unsigned char)modrm_i;
    unsigned char mod = (modrm >> 6) & 0b11;
    unsigned char op  = (modrm >> 3) & 0b111;  // ALU op selector
    unsigned char rm  = modrm & 0b111;

    const char *mnemonic = alu_imm_op(op);
    if (!mnemonic) {
        fprintf(out, "alu ; unsupported");
        return;
    }

    const char **regs = w ? reg16 : reg8;
    const char *size  = w ? "word" : "byte";

    /* ---- 1) destination (and displacement) ---- */
    char dst[64];

    switch (mod) {
    case 0b11:  // register
        snprintf(dst, sizeof(dst), "%s", regs[rm]);
        break;

    case 0b00:  // memory, no disp (except rm=110 => disp16 direct)
        if (rm == 0b110) {
            int lo = fgetc(in), hi = fgetc(in);
            if (lo == EOF || hi == EOF) { fprintf(stderr, "Unexpected EOF in disp16\n"); return; }
            unsigned short addr = (unsigned short)(lo | (hi << 8));
            snprintf(dst, sizeof(dst), "%s [%u]", size, addr);
        } else {
            snprintf(dst, sizeof(dst), "%s [%s]", size, ea_table[rm]);
        }
        break;

    case 0b01: { // disp8
        int b = fgetc(in);
        if (b == EOF) { fprintf(stderr, "Unexpected EOF in disp8\n"); return; }
        signed char disp8 = (signed char)b;

        snprintf(dst, sizeof(dst), "%s [%s %c %d]",
                 size, ea_table[rm],
                 disp8 >= 0 ? '+' : '-',
                 disp8 >= 0 ? disp8 : -disp8);
        break;
    }

    case 0b10: { // disp16
        int lo = fgetc(in), hi = fgetc(in);
        if (lo == EOF || hi == EOF) { fprintf(stderr, "Unexpected EOF in disp16\n"); return; }
        signed short disp16 = (signed short)(lo | (hi << 8));

        snprintf(dst, sizeof(dst), "%s [%s %c %d]",
                 size, ea_table[rm],
                 disp16 >= 0 ? '+' : '-',
                 disp16 >= 0 ? disp16 : -disp16);
        break;
    }

    default:
        fprintf(out, "%s ; not implemented", mnemonic);
        return;
    }

    /* ---- 2) immediate ---- */
    int imm;

    if (w == 0) {
        int b = fgetc(in);
        if (b == EOF) { fprintf(stderr, "Unexpected EOF in imm8\n"); return; }
        imm = (signed char)b;            // print like listing (signed)
    } else {
        if (s) {
            int b = fgetc(in);
            if (b == EOF) { fprintf(stderr, "Unexpected EOF in imm8(s)\n"); return; }
            imm = (signed char)b;        // sign-extended imm8
        } else {
            int lo = fgetc(in), hi = fgetc(in);
            if (lo == EOF || hi == EOF) { fprintf(stderr, "Unexpected EOF in imm16\n"); return; }
            imm = (signed short)(lo | (hi << 8));
        }
    }

    fprintf(out, "%s %s, %d", mnemonic, dst, imm);
}

void handle_alu_acc_imm(unsigned char first, FILE *in, FILE *out) {
    const char *mn = alu_acc_mnemonic(first);
    if (!mn) {
        fprintf(stderr, "handle_alu_acc_imm: not an acc-imm opcode: 0x%02X\n", first);
        return;
    }

    unsigned char w = first & 1; // 0=AL imm8, 1=AX imm16

    if (w == 0) {
        int b = fgetc(in);
        if (b == EOF) { fprintf(stderr, "Unexpected EOF in %s AL,imm8\n", mn); return; }
        signed char imm8 = (signed char)b;
        fprintf(out, "%s al, %d", mn, imm8);
    } else {
        int lo = fgetc(in);
        int hi = fgetc(in);
        if (lo == EOF || hi == EOF) { fprintf(stderr, "Unexpected EOF in %s AX,imm16\n", mn); return; }
        signed short imm16 = (signed short)(lo | (hi << 8));
        fprintf(out, "%s ax, %d", mn, imm16);
    }
}

void decode_file(FILE *in, FILE *out)
{
    int byte;

    while ((byte = fgetc(in)) != EOF) {
        unsigned char first = (unsigned char)byte;
        // unsigned char opcode = first >> 2;

        if (first == 0b00000100 || first == 0b00000101 ||   // add al/ax, imm
            first == 0b00101100 || first == 0b00101101 ||   // sub al/ax, imm
            first == 0b00111100 || first == 0b00111101) {   // cmp al/ax, imm
            handle_alu_acc_imm(first, in, out);
        }
        else if ( (first & 0b11111100) == 0b10001000 ) {           // 100010dw (MOV r/m <-> r)
            handle_mov_rm_r(first, in, out);
        }
        else if ( (first & 0b11110000) == 0b10110000 ) {           // 1011wreg (MOV imm -> reg)
            handle_mov_imm_r(first, in, out);
        }
        else if ( (first & 0b11111100) == 0b10000000 ) {           // 100000sw (ALU imm -> r/m)
            handle_alu_imm_rm(first, in, out);
        }
        else if ( (first & 0b11111100) == 0b00000000 ) {           // 000000dw (ADD r/m <-> r)
            handle_alu_rm_r("add", first, in, out);
        }
        else if ( (first & 0b11111100) == 0b00101000 ) {           // 001010dw (SUB r/m <-> r)
            handle_alu_rm_r("sub", first, in, out);
        }
        else if ( (first & 0b11111100) == 0b00111000 ) {           // 001110dw (CMP r/m <-> r)
            handle_alu_rm_r("cmp", first, in, out);
        }

        else if ( (first & 0b11110000) == 0b01110000 ) {                 // 0111cccc
            handle_jcc(first, in, out);
        }
        
        else if ( first == 0b11100010 || first == 0b11100001 ||
                first == 0b11100000 || first == 0b11100011 ) {    // loop/loopz/loopnz/jcxz
            handle_loop_family(first, in, out);
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



