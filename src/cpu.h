#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef enum {
  AX, BX, CX, DX, SP, BP, SI, DI, REG_UNKNOWN
} Reg16;

typedef enum {
  ZF, SF, F_UNKNOWN
} Flags;

typedef struct {
  uint16_t r[REG_UNKNOWN];
  u_int8_t f[F_UNKNOWN];
} CPU;

void cpu_init(CPU *cpu);
void cpu_print(const CPU *cpu);

#endif