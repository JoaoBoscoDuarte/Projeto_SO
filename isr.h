#ifndef ISR_H
#define ISR_H

#include "types.h"

typedef struct regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} regs_t;

void isr_handler(regs_t* r);

#endif