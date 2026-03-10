#include "gdt.h"

// Criamos uma lista com 3 "crachás" de acesso à memória.
struct gdt_entry gdt[3];

// Este é o cartão de visita que entregaremos ao processador 
// dizendo onde nossa lista (GDT) está guardada.
struct gdt_ptr gp;

// Essa função monta cada "crachá" (entrada) da tabela.
void gdt_set_gate(int num, unsigned int base, unsigned int limit, 
                  unsigned char access, unsigned char gran) 
{
    // O endereço de início (base) precisa ser fatiado em 3 pedaços 
    // porque o processador é antigo e lê os bits em lugares espalhados.
    gdt[num].base_low    = (base & 0xFFFF);         // Pedaço 1
    gdt[num].base_middle = (base >> 16) & 0xFF;     // Pedaço 2
    gdt[num].base_high   = (base >> 24) & 0xFF;     // Pedaço 3

    // O tamanho do segmento (limit) também é fatiado em 2.
    gdt[num].limit_low   = (limit & 0xFFFF);
    // Ajuste aqui:
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init() {
    // 1. Preparamos o ponteiro que o processador vai ler depois.
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1; // Tamanho da tabela
    gp.base  = (unsigned int)&gdt;                 // Onde ela está na RAM

    // 2. CRIAÇÃO DOS SEGMENTOS (Os cartões de acesso):

    // Entrada 0: O "Crachá Vazio". O processador exige que o primeiro 
    // seja nulo por segurança. Se alguém usar, o PC reinicia.
    gdt_set_gate(0, 0, 0, 0, 0);

    // Entrada 1: Código do Kernel.
    // Dá permissão para o processador EXECUTAR ordens.
    // Base 0, Limite 4GB, Acesso 0x9A (Código), Granularidade 0xCF (4KB/32bits)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Entrada 2: Dados do Kernel.
    // Dá permissão para o processador GUARDAR valores na memória (leitura/escrita).
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // envia para o processador 
    gdt_flush((unsigned int)&gp);
}