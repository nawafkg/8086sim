#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef enum {
  AX, BX, CX, DX, SP, BP, SI, DI, REG_UNKNOWN
} Reg16;

typedef struct {
  uint16_t r[8];
} CPU;

void cpu_init(CPU *cpu);
void cpu_print(const CPU *cpu);

#endif