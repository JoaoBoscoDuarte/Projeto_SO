#ifndef INCLUDE_PROCESS_H
#define INCLUDE_PROCESS_H

/*
 * process.h — Estrutura de processo e setup para user mode
 *
 * Descreve um processo simples: um page directory próprio, um
 * ponto de entrada (eip) e um stack pointer inicial (esp).
 */

/*
 * Estrutura mínima de um processo.
 * Expandível no futuro para suportar múltiplos processos.
 */
struct process {
    unsigned int page_directory_phys; /* endereço FÍSICO do page directory */
    unsigned int eip;                 /* instruction pointer inicial        */
    unsigned int esp;                 /* stack pointer inicial (user stack) */
};

/*
 * process_create(code_phys, code_size)
 *
 * Cria um novo processo a partir de um binário flat já carregado na
 * memória física em 'code_phys' com tamanho 'code_size' bytes.
 *
 * O que esta função faz:
 *   1. Aloca um frame para o page directory do processo
 *   2. Zera o PD e copia as entradas do kernel (índices 768+)
 *   3. Aloca frame(s) para o código e copia o binário
 *   4. Mapeia o código em virtual 0x00000000 com PAGE_USER_RW
 *   5. Aloca um frame para a stack do user
 *   6. Mapeia a stack em virtual 0xBFFFF000 com PAGE_USER_RW
 *
 * Retorna a struct process preenchida.
 * Em caso de falha de alocação, retorna uma struct com eip=0 e esp=0.
 */
struct process process_create(unsigned int code_phys, unsigned int code_size);

/*
 * enter_usermode(eip, esp, page_directory_phys)
 *
 * Executa a transição ring 0 → ring 3 via iret.
 * Definida em src/boot/usermode.s — não retorna.
 */
extern void enter_usermode(unsigned int eip,
                           unsigned int esp,
                           unsigned int page_directory_phys);

#endif /* INCLUDE_PROCESS_H */
