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
    tss_set_kernel_stack(next->kernel_stack_top);
    current_process = next;
    context_switch(&old->esp, next->esp);
}

void yield(void)
{
    schedule();
}
