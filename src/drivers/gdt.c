#include "gdt.h"

/*
 * GDT com 6 entradas:
 *   0 (0x00): null descriptor       — obrigatório pelo x86
 *   1 (0x08): kernel code segment   — DPL=0, Execute-Read
 *   2 (0x10): kernel data segment   — DPL=0, Read-Write
 *   3 (0x18): user code segment     — DPL=3, Execute-Read  (Capítulo 11)
 *   4 (0x20): user data segment     — DPL=3, Read-Write    (Capítulo 11)
 *   5 (0x28): TSS descriptor        — preenchido por tss_init()
 */
struct gdt_entry gdt[6];

struct gdt_ptr gp;

void gdt_set_gate(int num, unsigned int base, unsigned int limit,
                  unsigned char access, unsigned char gran)
{
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void)
{
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (unsigned int)&gdt;

    /* Entrada 0: null — obrigatório */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Entrada 1: kernel code — Present | DPL=0 | Code | Execute-Read */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Entrada 2: kernel data — Present | DPL=0 | Data | Read-Write */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Entrada 3: user code — Present | DPL=3 | Code | Execute-Read (0xFA) */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* Entrada 4: user data — Present | DPL=3 | Data | Read-Write (0xF2) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Entrada 5: TSS descriptor — preenchida por tss_init() após gdt_init() */
    gdt_set_gate(5, 0, 0, 0, 0);

    gdt_flush((unsigned int)&gp);
}