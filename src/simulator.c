#include <stdio.h>
#include <stdlib.h>
#include "simulator.h"
#include "string.h"
#include <ctype.h>

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_CMP,
    OP_MOV,
    OP_UNKNOWN
} Op;

Op parse_op(const char *w) {
    if (strcmp(w, "add") == 0) return OP_ADD;
    if (strcmp(w, "sub") == 0) return OP_SUB;
    if (strcmp(w, "cmp") == 0) return OP_CMP;
    if (strcmp(w, "mov") == 0) return OP_MOV;
    return OP_UNKNOWN;
}

Reg16 parse_reg(const char *w) {
    if ((strcmp(w, "ax") == 0) || (strcmp(w, "ax,") == 0)) return AX;
    if ((strcmp(w, "bx") == 0) || (strcmp(w, "bx,") == 0)) return BX;
    if ((strcmp(w, "cx") == 0) || (strcmp(w, "cx,") == 0)) return CX;
    if ((strcmp(w, "dx") == 0) || (strcmp(w, "dx,") == 0)) return DX;
    if ((strcmp(w, "sp") == 0) || (strcmp(w, "sp,") == 0)) return SP;
    if ((strcmp(w, "bp") == 0) || (strcmp(w, "bp,") == 0)) return BP;
    if ((strcmp(w, "si") == 0) || (strcmp(w, "si,") == 0)) return SI;
    if ((strcmp(w, "di") == 0) || (strcmp(w, "di,") == 0)) return DI;
    return REG_UNKNOWN;
}

char line[128];
char word[32];

char* readline(FILE *f, char *buf, size_t cap) {
    return fgets(buf, cap, f);
}

char *consume_word(char *p) {
    while (*p && isspace((unsigned char)*p)) p++;

    char *w = word;
    while (*p && !isspace((unsigned char)*p)) *w++ = *p++;
    *w = '\0';

    while (*p && isspace((unsigned char)*p)) p++;

    return p;
}

void simulate_mov(CPU *cpu) {
    char *p = &line;
    p = consume_word(p);
    p = consume_word(p);
    Reg16 reg1 = parse_reg(&word);
    long imm;

    p = consume_word(p);
    Reg16 reg2 = parse_reg(&word);
    if(reg2 == REG_UNKNOWN) {
       imm = strtol(word, NULL, 0);
       cpu->r[reg1] = imm;
    } else {
        cpu->r[reg1] = cpu->r[reg2];
    }
}



void simulate(CPU *cpu, FILE *in) {

    while(readline(in, line, sizeof(line))){
        sscanf(line, "%31s", word);
        Op op = parse_op(&word);
        switch (op) {
        case OP_MOV:
            simulate_mov(cpu);
            break;
        
        default:
            break;
        }
    }
}