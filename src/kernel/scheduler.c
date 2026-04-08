#include "scheduler.h"
#include "process.h"
#include "tss.h"

static process_t *find_next_ready(void)
{
    unsigned int i;
    unsigned int start;

    if (!current_process)
        return (process_t *)0;

    start = (unsigned int)(current_process - process_table);

    for (i = 1; i < MAX_PROCESSES; i++) {
        unsigned int idx = (start + i) % MAX_PROCESSES;
        if (process_table[idx].state == PROC_READY)
            return &process_table[idx];
    }

    return (process_t *)0;
}

/* ============================================================================
 * schedule — troca de contexto para o próximo processo pronto
 * ========================================================================== */
void schedule(void)
{
    process_t *next;
    process_t *old;

    next = find_next_ready();
    if (!next || next == current_process)
        return;

    old = current_process;

    old->state  = PROC_READY;
    next->state = PROC_RUNNING;

    /* Atualiza o TSS para que interrupções em ring 3 usem a kernel stack
     * correta do processo que está prestes a executar. */
    if (next->kernel_stack_top != 0) {
        tss_set_kernel_stack(next->kernel_stack_top);
    }

    /* Troca de contexto — salva old->esp, carrega next->esp */
    current_process = next;

    /* Se os processos tiverem mapas de memória diferentes, trocamos o CR3 */
    if (old->page_directory_phys != next->page_directory_phys) {
        asm volatile("mov %0, %%cr3" : : "r"(next->page_directory_phys));
    }

    /* Salta para a nova pilha (chama o seu switch.s) */
    context_switch(&old->esp, next->esp);
}

void yield(void)
{
    schedule();
}
