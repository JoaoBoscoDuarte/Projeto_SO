#include "scheduler.h"
#include "process.h"
#include "tss.h"

/* ============================================================================
 * scheduler.c — Scheduler cooperativo round-robin
 *
 * Funciona apenas com kernel stacks por processo (Etapa 4).
 * O kernel stack pointer salvo fica em process_t.esp.
 *
 * Fluxo de um context switch:
 *   yield() → schedule() → context_switch(&old->esp, next->esp)
 *
 * context_switch() (em switch.s):
 *   - Salva ebp/ebx/esi/edi na kernel stack atual
 *   - Escreve ESP atual em old->esp
 *   - Carrega next->esp como novo ESP
 *   - Restaura ebp/ebx/esi/edi da nova kernel stack
 *   - "ret" continua no contexto do processo seguinte
 * ========================================================================== */

/* ============================================================================
 * find_next_ready — busca o próximo processo READY após current_process
 *
 * Implementa round-robin: começa no slot após o processo corrente e
 * dá uma volta completa pela tabela.
 * Retorna NULL se não houver nenhum processo READY (além do atual).
 * ========================================================================== */
static process_t *find_next_ready(void)
{
    unsigned int i;
    unsigned int start;

    if (!current_process)
        return (process_t *)0;

    /* Índice do processo atual na tabela */
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
        return; /* nada a fazer */

    old = current_process;

    /* Atualiza estados */
    old->state  = PROC_READY;
    next->state = PROC_RUNNING;

    /* Atualiza o TSS para que interrupções em ring 3 usem a kernel stack
     * correta do processo que está prestes a executar. */
    tss_set_kernel_stack(next->kernel_stack_top);

    /* Troca de contexto — salva old->esp, carrega next->esp */
    current_process = next;
    context_switch(&old->esp, next->esp);

    /* Execução retorna aqui apenas quando 'old' for reescalonado */
}

/* ============================================================================
 * yield — interface pública para ceder a CPU voluntariamente
 * ========================================================================== */
void yield(void)
{
    schedule();
}
