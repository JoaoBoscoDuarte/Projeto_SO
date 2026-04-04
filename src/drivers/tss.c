#include "tss.h"
#include "gdt.h"

/*
 * tss.c — Task State Segment (Capítulos 11 e 13 do Little OS Book)
 *
 * O TSS (Task State Segment) é uma estrutura que o hardware x86 usa para
 * salvar/restaurar o estado da CPU durante trocas de privilégio.
 *
 * Neste kernel, usamos o TSS apenas para informar ao CPU qual kernel stack
 * deve ser usada ao receber uma interrupção enquanto executando em ring 3.
 * Os campos ss0 e esp0 são os únicos que precisam ser configurados.
 */

static struct tss_entry tss;

/*
 * tss_init(kernel_ss, kernel_esp)
 *
 * 1. Zera toda a struct TSS
 * 2. Define ss0 e esp0 (kernel stack para retorno de interrupções de ring 3)
 * 3. Instala o descritor TSS na entrada 5 da GDT
 * 4. Chama tss_flush() para executar 'ltr' e carregar o TSS no registro TR
 */
void tss_init(unsigned int kernel_ss, unsigned int kernel_esp)
{
    unsigned int i;
    unsigned char *p = (unsigned char *)&tss;
    unsigned int base;
    unsigned int limit;

    /* Zera toda a estrutura TSS */
    for (i = 0; i < sizeof(tss); i++)
        p[i] = 0;

    /* Configura a kernel stack: quando o CPU recebe uma interrupção em ring 3,
     * ele consulta ss0/esp0 para saber para qual stack trocar. */
    tss.ss0  = kernel_ss;
    tss.esp0 = kernel_esp;

    /*
     * Instala o descritor TSS na GDT[5].
     *
     * access = 0x89:
     *   bit 7 (P)    = 1  — presente na memória
     *   bit 6-5 (DPL)= 00 — privilege level 0
     *   bit 4 (S)    = 0  — system descriptor (não código/dados)
     *   bit 3-0 (Type)= 1001 — 32-bit TSS Available
     *
     * gran = 0x40:
     *   bit 6 (D/B)  = 1  — 32-bit segment
     *   outros bits  = 0
     */
    base  = (unsigned int)&tss;
    limit = sizeof(tss) - 1;
    gdt_set_gate(5, base, limit, 0x89, 0x40);

    /* Carrega o TSS no registro TR (Task Register) */
    tss_flush();
}

/*
 * tss_set_kernel_stack(esp)
 *
 * Atualiza esp0 no TSS. Chamar esta função ao trocar de processo garante
 * que o CPU use a kernel stack correta ao receber uma interrupção de ring 3.
 */
void tss_set_kernel_stack(unsigned int esp)
{
    tss.esp0 = esp;
}
