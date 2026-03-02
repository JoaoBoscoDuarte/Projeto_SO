#include "gdt.h"

// ============================================================================
// GDT - Global Descriptor Table
// ============================================================================
// Em modo protegido x86, a memória é acessada através de SEGMENTOS.
// A GDT define esses segmentos (código, dados, pilha).
// Cada segmento tem: base (início), limite (tamanho), permissões.
// ============================================================================

// Array com 3 entradas: null, código, dados
struct gdt_entry gdt[3];

// Ponteiro para a GDT (usado pela instrução LGDT)
struct gdt_ptr gp;

// ============================================================================
// gdt_set_gate - Configura uma entrada da GDT
// ============================================================================
// num: índice na GDT (0, 1, 2...)
// base: endereço inicial do segmento (32 bits)
// limit: tamanho do segmento (20 bits)
// access: flags de acesso (presente, privilégio, tipo)
// gran: granularidade e flags adicionais
void gdt_set_gate(int num, unsigned int base, unsigned int limit, 
                  unsigned char access, unsigned char gran) 
{
    // Base é dividida em 3 partes (compatibilidade com 8086)
    gdt[num].base_low    = (base & 0xFFFF);         // Bits 0-15
    gdt[num].base_middle = (base >> 16) & 0xFF;     // Bits 16-23
    gdt[num].base_high   = (base >> 24) & 0xFF;     // Bits 24-31

    // Limite é dividido em 2 partes (20 bits no total)
    gdt[num].limit_low   = (limit & 0xFFFF);        // Bits 0-15
    gdt[num].granularity = (limit >> 16) & 0x0F;    // Bits 16-19

    // Granularidade: bits 4-7 contêm flags adicionais
    gdt[num].granularity |= gran & 0xF0;
    
    // Access byte: define tipo e permissões do segmento
    gdt[num].access = access;
}

// ============================================================================
// gdt_init - Inicializa a GDT
// ============================================================================
// Cria 3 segmentos: null, código do kernel, dados do kernel
void gdt_init() {
    // Configura o ponteiro da GDT
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;  // Tamanho - 1
    gp.base  = (unsigned int)&gdt;                  // Endereço da GDT

    // Entrada 0: NULL DESCRIPTOR (obrigatório, nunca usado)
    gdt_set_gate(0, 0, 0, 0, 0);

    // Entrada 1 (offset 0x08): KERNEL CODE SEGMENT
    // Base: 0x00000000, Limite: 4GB (0xFFFFFFFF)
    // Access: 0x9A = 10011010b
    //   Bit 7: Present (segmento está na memória)
    //   Bits 6-5: Privilege Level 0 (kernel)
    //   Bit 4: Descriptor type (1 = código/dados)
    //   Bit 3: Executable (1 = código)
    //   Bit 2: Direction (0 = cresce para cima)
    //   Bit 1: Readable (1 = pode ler código)
    //   Bit 0: Accessed (0 = não acessado ainda)
    // Gran: 0xCF = 11001111b
    //   Bit 7: Granularity (1 = limite em páginas de 4KB)
    //   Bit 6: Size (1 = segmento 32-bit)
    //   Bits 3-0: parte alta do limite
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Entrada 2 (offset 0x10): KERNEL DATA SEGMENT
    // Access: 0x92 = 10010010b
    //   Similar ao código, mas Bit 3 = 0 (não executável, dados)
    //   Bit 1 = 1 (writable, pode escrever)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Carrega a GDT no processador (função em assembly)
    gdt_flush((unsigned int)&gp);
}
