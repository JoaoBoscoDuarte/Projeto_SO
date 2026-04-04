#ifndef INCLUDE_SCHEDULER_H
#define INCLUDE_SCHEDULER_H

/* ============================================================================
 * scheduler.h — Scheduler cooperativo simples
 *
 * Uso:
 *   - Um processo chama yield() para ceder a CPU voluntariamente.
 *   - schedule() escolhe o próximo processo READY em round-robin.
 *   - context_switch() (em switch.s) salva/restaura o estado da kernel stack.
 * ========================================================================== */

/*
 * schedule — encontra o próximo processo READY e executa context_switch.
 * Se não houver outro processo pronto, retorna sem trocar de contexto.
 */
void schedule(void);

/*
 * yield — cede a CPU ao próximo processo disponível.
 * Equivalente a chamar schedule() explicitamente.
 */
void yield(void);

/*
 * context_switch — salva o ESP do processo atual em *old_esp,
 * e restaura a execução a partir de new_esp.
 * Implementada em switch.s.
 */
extern void context_switch(unsigned int *old_esp_ptr, unsigned int new_esp);

#endif /* INCLUDE_SCHEDULER_H */
