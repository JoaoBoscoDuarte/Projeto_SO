#ifndef INCLUDE_TSS_H
#define INCLUDE_TSS_H

/*
 * tss.h — Task State Segment
 *
 * O TSS é necessário para que o CPU saiba qual kernel stack usar ao
 * receber uma interrupção enquanto executa em ring 3 (user mode).
 *
 * Quando ocorre uma interrupção inter-privilege (ring 3 → ring 0):
 *   1. O CPU lê ss0 e esp0 do TSS
 *   2. Troca SS e ESP para a kernel stack
 *   3. Pusha ss_user, esp_user, eflags, cs_user, eip_user na kernel stack
 *   4. Salta para o handler da IDT
 *
 * Sem TSS configurado, qualquer interrupção em user mode causa triple fault.
 */

/*
 * Estrutura do TSS para modo protegido 32-bit (Intel Vol. 3A, Figura 7-2).
 * Todos os campos são de 32 bits exceto trap e iomap_base (16 bits).
 * O __attribute__((packed)) garante que o compilador não adiciona padding —
 * o CPU acessa os campos por offset fixo, não por nome.
 */
struct tss_entry {
    unsigned int  prev_tss;    /* seletor da tarefa anterior (não usado) */
    unsigned int  esp0;        /* kernel stack pointer (ring 0)          */
    unsigned int  ss0;         /* kernel stack segment (ring 0)          */
    unsigned int  esp1;        /* stack pointer ring 1 (não usado)       */
    unsigned int  ss1;         /* stack segment ring 1 (não usado)       */
    unsigned int  esp2;        /* stack pointer ring 2 (não usado)       */
    unsigned int  ss2;         /* stack segment ring 2 (não usado)       */
    unsigned int  cr3;         /* page directory (não usado — gerenciado pelo kernel) */
    unsigned int  eip;         /* instruction pointer salvo (não usado)  */
    unsigned int  eflags;      /* flags salvas (não usado)               */
    unsigned int  eax;         /* registradores salvos (não usados)      */
    unsigned int  ecx;
    unsigned int  edx;
    unsigned int  ebx;
    unsigned int  esp;
    unsigned int  ebp;
    unsigned int  esi;
    unsigned int  edi;
    unsigned int  es;          /* segment selectors salvos (não usados)  */
    unsigned int  cs;
    unsigned int  ss;
    unsigned int  ds;
    unsigned int  fs;
    unsigned int  gs;
    unsigned int  ldt;         /* LDT segment selector (não usado)       */
    unsigned short trap;       /* debug trap flag                        */
    unsigned short iomap_base; /* offset do I/O permission bitmap        */
} __attribute__((packed));

/*
 * tss_init(kernel_ss, kernel_esp)
 *
 * Inicializa o TSS com a kernel stack e instala o descritor TSS na GDT[5].
 * Deve ser chamada APÓS gdt_init() (porque modifica a GDT) e antes de
 * entrar em user mode.
 *
 * kernel_ss  : seletor do segmento de dados do kernel = GDT_KERNEL_DATA (0x10)
 * kernel_esp : endereço do topo da kernel stack
 */
void tss_init(unsigned int kernel_ss, unsigned int kernel_esp);

/*
 * tss_set_kernel_stack(esp)
 *
 * Atualiza esp0 no TSS para que o CPU use a stack correta ao receber
 * uma interrupção de ring 3. Deve ser chamada ao trocar de processo.
 */
void tss_set_kernel_stack(unsigned int esp);

/* tss_flush — definida em tss.s; executa 'ltr' para carregar o TSS */
extern void tss_flush(void);

#endif /* INCLUDE_TSS_H */
