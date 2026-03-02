#include "idt.h"

// Tipos base para não precisarmos do <stdint.h> se você não tiver
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// A estrutura exata que o processador x86 exige para cada entrada da IDT
struct idt_entry {
    uint16_t base_lo;   // Os 16 bits mais baixos do endereço da função
    uint16_t sel;       // Seletor do segmento do Kernel na GDT (geralmente 0x08)
    uint8_t  always0;   // Esse byte deve ser sempre zero
    uint8_t  flags;     // Flags indicando que é um portão de interrupção (0x8E)
    uint16_t base_hi;   // Os 16 bits mais altos do endereço da função
} __attribute__((packed));

struct idt_entry idt[256]; // Criamos as 256 posições

// Funções que vão vir lá do nosso Assembly (passo 3)
extern void idt_load(uint32_t idt_ptr);
extern void interrupt_handler_33(void);

// Função auxiliar para preencher uma posição da lista
void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags;
}

void idt_init(void) {
    // 1. Primeiro, limpamos TODA a tabela com zeros.
    // Isso evita que interrupções não tratadas (como o timer) causem um crash.
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0); 
    }

    // 2. Agora, configuramos especificamente a interrupção 33 (Teclado).
    // Fazemos isso ANTES de carregar a IDT.
    // No idt_init:
    idt_set_gate(33, (uint32_t)interrupt_handler_33, 0x08, 0x8E);

    // 3. Preparamos o ponteiro que a CPU vai usar.
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idt_pointer = {
        .limit = sizeof(struct idt_entry) * 256 - 1,
        .base  = (uint32_t)&idt
    };

    // 4. Finalmente, chamamos o Assembly para carregar e dar o 'sti'.
    idt_load((uint32_t)&idt_pointer);
}