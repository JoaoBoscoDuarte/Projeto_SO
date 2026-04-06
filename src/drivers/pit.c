#include "pit.h"
#include "process.h"
#include "scheduler.h"

extern void outb(unsigned short port, unsigned char data);

#define PIT_CMD  0x43
#define PIT_CH0  0x40
#define PIT_FREQ 1193182

#define PREEMPT_INTERVAL 10  /* a cada 10 ticks = 100ms */

static volatile unsigned int system_ticks = 0;

void pit_init(unsigned int freq_hz)
{
    unsigned int divisor = PIT_FREQ / freq_hz;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (unsigned char)(divisor & 0xFF));
    outb(PIT_CH0, (unsigned char)((divisor >> 8) & 0xFF));
}

void pit_handler_c(void)
{
    system_ticks++;
    if (current_process)
        current_process->ticks_total++;

    /* Envia o aviso (EOI) ao PIC ANTES de trocar de contexto! 
       Se o fizer depois, o timer bloqueia no outro processo. */
    outb(0x20, 0x20);

    /* PREEMPÇÃO: Força o CPU a passar para o próximo processo */
    if (system_ticks % PREEMPT_INTERVAL == 0) {
        schedule();
    }
}

unsigned int pit_get_ticks(void)
{
    return system_ticks;
}

void sleep_ticks(unsigned int t)
{
    unsigned int end = system_ticks + t;
    while (system_ticks < end)
        asm volatile("hlt");
}
