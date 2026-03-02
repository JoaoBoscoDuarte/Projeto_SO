#include "gdt.h"

// GDT com 3 entradas: null, code, data
struct gdt_entry gdt[3];
struct gdt_ptr gp;

void gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    // Ajuste aqui:
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init() {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (unsigned int)&gdt;

    /* 0x00: Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* 0x08: Kernel Code Segment (Base 0, Limit 4GB, PL0) */
    /* Access: 0x9A (10011010b) -> Present, Ring 0, Code, Exec/Read */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* 0x10: Kernel Data Segment (Base 0, Limit 4GB, PL0) */
    /* Access: 0x92 (10010010b) -> Present, Ring 0, Data, Read/Write */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_flush((unsigned int)&gp);
}