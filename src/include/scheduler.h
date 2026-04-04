#ifndef INCLUDE_SCHEDULER_H
#define INCLUDE_SCHEDULER_H

/* ============================================================================
 * scheduler.h — Scheduler cooperativo simples
 * ========================================================================== */

void schedule(void);
void yield(void);

extern void context_switch(unsigned int *old_esp_ptr, unsigned int new_esp);

#endif /* INCLUDE_SCHEDULER_H */
