#include "cpu.h"
#include <stdio.h>

void cpu_init(CPU *cpu) {
    for(int i = 0; i < 8; i++) {
        cpu->r[i] = 0;
    }
}

void cpu_print(const CPU *cpu)
{
    printf(
        "AX=%04X  BX=%04X  CX=%04X  DX=%04X\n"
        "SP=%04X  BP=%04X  SI=%04X  DI=%04X\n"
        "ZF=%d  SF=%d\n",
        cpu->r[AX], cpu->r[BX], cpu->r[CX], cpu->r[DX],
        cpu->r[SP], cpu->r[BP], cpu->r[SI], cpu->r[DI],
        cpu->f[ZF], cpu->f[SF]
    );
}

