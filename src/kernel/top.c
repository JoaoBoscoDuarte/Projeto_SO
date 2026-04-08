#include "top.h"
#include "fb.h"
#include "process.h"
#include "pit.h"
#include "keyboard.h"
#include "pfa.h"
#include "kheap.h"
#include "cpuid.h"

/* ============================================================================
 * Helpers de formatação — sem depender de kprintf para posicionamento
 * ========================================================================== */

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

/* Escreve "XXXX KB" a partir de (row, col) */
static void write_kb(unsigned int row, unsigned int col,
                     unsigned int kb, unsigned char fg)
{
    char buf[12];
    fb_write_str_at(row, col, uint_to_str(kb, buf, sizeof(buf)), fg, FB_BLACK);
    col += 8;
    fb_write_str_at(row, col, " KB", fg, FB_BLACK);
}

/* Escreve "XX%" a partir de (row, col) */
static void write_pct(unsigned int row, unsigned int col,
                      unsigned int pct, unsigned char fg)
{
    char buf[8];
    fb_write_str_at(row, col, uint_to_str(pct, buf, sizeof(buf)), fg, FB_BLACK);
    fb_write_str_at(row, col + 3, "%  ", fg, FB_BLACK);
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

/* ============================================================================
 * Snapshot de ticks para cálculo de CPU%
 *
 * A cada refresh guardamos os ticks de cada processo e o total do sistema.
 * CPU% = (delta_proc / delta_total) * 100
 * ========================================================================== */
static unsigned int snap_proc[MAX_PROCESSES];
static unsigned int snap_total = 0;

static void snapshot_take(void)
{
    unsigned int i;
    snap_total = pit_get_ticks();
    for (i = 0; i < MAX_PROCESSES; i++)
        snap_proc[i] = process_table[i].ticks_total;
}

static unsigned int cpu_pct(unsigned int idx, unsigned int total_now)
{
    unsigned int delta_total = total_now - snap_total;
    unsigned int delta_proc;

    if (delta_total == 0)
        return 0;

    delta_proc = process_table[idx].ticks_total - snap_proc[idx];
    if (delta_proc > delta_total)
        delta_proc = delta_total;

    return (delta_proc * 100) / delta_total;
}

/* ============================================================================
 * top_draw — redesenha a tela inteira
 * ========================================================================== */
static void top_draw(void)
{
    unsigned int i;
    unsigned int row = 0;
    char buf[52];
    unsigned int total_now = pit_get_ticks();

    /* --- Linha 0: título --- */
    fb_clear_line(0);
    fb_write_str_at(0, 0,
        "=== TOP ================================================",
        FB_CYAN, FB_BLACK);

    /* --- Linha 1: CPU vendor/brand --- */
    fb_clear_line(1);
    fb_write_str_at(1, 0, "CPU: ", FB_LIGHT_GREY, FB_BLACK);
    cpuid_brand(buf);
    fb_write_str_at(1, 5, buf, FB_WHITE, FB_BLACK);

    /* --- Linha 2: Uptime --- */
    fb_clear_line(2);
    fb_write_str_at(2, 0, "Uptime: ", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(2, 8,
        uint_to_str(total_now, buf, sizeof(buf)),
        FB_WHITE, FB_BLACK);
    fb_write_str_at(2, 16, " ticks  (", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(2, 25,
        uint_to_str(total_now / 100, buf, sizeof(buf)),
        FB_WHITE, FB_BLACK);
    fb_write_str_at(2, 29, " s)", FB_LIGHT_GREY, FB_BLACK);

    /* --- Linha 3: RAM --- */
    {
        unsigned int total_kb = pfa_total_frames() * 4;
        unsigned int free_kb  = pfa_free_frames()  * 4;
        unsigned int used_kb  = total_kb - free_kb;

        fb_clear_line(3);
        fb_write_str_at(3, 0,  "RAM:  total=", FB_LIGHT_GREY, FB_BLACK);
        write_kb(3, 12, total_kb, FB_WHITE);
        fb_write_str_at(3, 22, " used=", FB_LIGHT_GREY, FB_BLACK);
        write_kb(3, 28, used_kb, FB_LIGHT_RED);
        fb_write_str_at(3, 38, " free=", FB_LIGHT_GREY, FB_BLACK);
        write_kb(3, 44, free_kb, FB_LIGHT_GREEN);
    }

    /* --- Linha 4: Heap do kernel --- */
    {
        unsigned int heap_used  = kheap_used_bytes();
        unsigned int heap_total = kheap_total_bytes();

        fb_clear_line(4);
        fb_write_str_at(4, 0,  "Heap: total=", FB_LIGHT_GREY, FB_BLACK);
        write_kb(4, 12, heap_total / 1024, FB_WHITE);
        fb_write_str_at(4, 22, " used=", FB_LIGHT_GREY, FB_BLACK);
        write_kb(4, 28, heap_used / 1024, FB_LIGHT_RED);
    }

    /* --- Linha 5: separador --- */
    fb_clear_line(5);
    fb_write_str_at(5, 0,
        "--------------------------------------------------------",
        FB_DARK_GREY, FB_BLACK);

    /* --- Linha 6: cabeçalho da tabela --- */
    fb_clear_line(6);
    fb_write_str_at(6,  0, "PID", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(6,  4, "NOME            ", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(6, 21, "ESTADO ", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(6, 29, "CPU%", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(6, 34, "MEM(KB)", FB_LIGHT_GREY, FB_BLACK);
    fb_write_str_at(6, 42, "TICKS", FB_LIGHT_GREY, FB_BLACK);

    /* --- Linha 7: separador --- */
    fb_clear_line(7);
    fb_write_str_at(7, 0,
        "---  ---------------  -------  ----  -------  -----",
        FB_DARK_GREY, FB_BLACK);

    row = 8;

    /* --- Linhas 8+: processos --- */
    for (i = 0; i < MAX_PROCESSES && row < 23; i++) {
        process_t *p = &process_table[i];
        if (p->state == PROC_UNUSED)
            continue;

        fb_clear_line(row);

        /* PID */
        fb_write_str_at(row, 0,
            uint_to_str(p->pid, buf, sizeof(buf)),
            FB_WHITE, FB_BLACK);

        /* Nome */
        fb_write_str_at(row, 4, p->name, FB_LIGHT_GREEN, FB_BLACK);

        /* Estado */
        fb_write_str_at(row, 21, state_abbrev(p->state),
            p->state == PROC_RUNNING ? FB_LIGHT_GREEN : FB_LIGHT_BROWN,
            FB_BLACK);

        /* CPU% */
        write_pct(row, 29, cpu_pct(i, total_now), FB_LIGHT_CYAN);

        /* Memória */
        write_kb(row, 34, p->mem_frames * 4, FB_WHITE);

        /* Ticks */
        fb_write_str_at(row, 42,
            uint_to_str(p->ticks_total, buf, sizeof(buf)),
            FB_WHITE, FB_BLACK);

        row++;
    }

    /* Limpa linhas de processo não usadas */
    while (row < 24)
        fb_clear_line(row++);

    /* --- Linha 24: rodapé --- */
    fb_clear_line(24);
    fb_write_str_at(24, 0,
        "Pressione 'q' para sair",
        FB_DARK_GREY, FB_BLACK);

    /* Atualiza snapshot para o próximo refresh */
    snapshot_take();
}

/* ============================================================================
 * top_run
 * ========================================================================== */
void top_run(void)
{
    fb_clear();
    snapshot_take();

    while (1) {
        top_draw();

        unsigned int target = pit_get_ticks() + 50; /* ~500ms */
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
