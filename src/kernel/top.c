#include "top.h"
#include "fb.h"
#include "process.h"
#include "pit.h"
#include "keyboard.h"
#include "printf.h"

/* Converte unsigned int para string decimal em buf, retorna ponteiro para buf */
static char *uint_to_str(unsigned int val, char *buf, unsigned int buflen)
{
    unsigned int i = buflen - 1;
    buf[i] = '\0';
    if (val == 0) {
        buf[--i] = '0';
    } else {
        while (val && i > 0) {
            buf[--i] = '0' + (val % 10);
            val /= 10;
        }
    }
    return &buf[i];
}

static const char *state_abbrev(proc_state_t s)
{
    switch (s) {
        case PROC_UNUSED:  return "UNUSED ";
        case PROC_READY:   return "READY  ";
        case PROC_RUNNING: return "RUNNING";
        case PROC_BLOCKED: return "BLOCKED";
        case PROC_ZOMBIE:  return "ZOMBIE ";
        default:           return "???????";
    }
}

static void top_draw(void)
{
    unsigned int i;
    unsigned int row = 0;
    char numbuf[12];

    fb_write_str_at(row++, 0,
        "=== TOP - Processos do Sistema ===              ",
        FB_CYAN, FB_BLACK);

    fb_write_str_at(row++, 0,
        "PID  NOME             ESTADO   TICKS           ",
        FB_LIGHT_GREY, FB_BLACK);

    fb_write_str_at(row++, 0,
        "---  ---------------  -------  -----           ",
        FB_DARK_GREY, FB_BLACK);

    for (i = 0; i < MAX_PROCESSES && row < 23; i++) {
        process_t *p = &process_table[i];
        if (p->state == PROC_UNUSED)
            continue;

        fb_clear_line(row);

        /* PID */
        fb_write_str_at(row, 0,
            uint_to_str(p->pid, numbuf, sizeof(numbuf)),
            FB_WHITE, FB_BLACK);

        /* Nome (max 15 chars) */
        fb_write_str_at(row, 5, p->name, FB_LIGHT_GREEN, FB_BLACK);

        /* Estado */
        fb_write_str_at(row, 23, state_abbrev(p->state),
            FB_LIGHT_BROWN, FB_BLACK);

        /* Ticks */
        fb_write_str_at(row, 33,
            uint_to_str(p->ticks_total, numbuf, sizeof(numbuf)),
            FB_WHITE, FB_BLACK);

        row++;
    }

    /* Limpa linhas restantes até a 23 */
    while (row < 24)
        fb_clear_line(row++);

    /* Linha de rodapé */
    fb_clear_line(24);
    fb_write_str_at(24, 0, "Pressione 'q' para sair | Uptime: ",
        FB_DARK_GREY, FB_BLACK);
    fb_write_str_at(24, 35,
        uint_to_str(pit_get_ticks(), numbuf, sizeof(numbuf)),
        FB_DARK_GREY, FB_BLACK);
    fb_write_str_at(24, 43, " ticks", FB_DARK_GREY, FB_BLACK);
}

void top_run(void)
{
    fb_clear();
    while (1) {
        top_draw();

        /* Aguarda ~500ms (50 ticks a 100Hz), verificando 'q' a cada tick */
        unsigned int target = pit_get_ticks() + 50;
        while (pit_get_ticks() < target) {
            char c = kbd_try_getchar();
            if (c == 'q') {
                fb_clear();
                return;
            }
            asm volatile("hlt");
        }
    }
}
