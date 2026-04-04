#include "shell.h"
#include "keyboard.h"
#include "fb.h"
#include "printf.h"
#include "string.h"
#include "process.h"
#include "pit.h"
#include "top.h"

extern void outb(unsigned short port, unsigned char data);

static const char *state_name(proc_state_t s)
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

static void cmd_help(void)
{
    kprintf(OUTPUT_FB, "Comandos disponiveis:\n");
    kprintf(OUTPUT_FB, "  help   - lista os comandos\n");
    kprintf(OUTPUT_FB, "  clear  - limpa a tela\n");
    kprintf(OUTPUT_FB, "  ps     - lista processos\n");
    kprintf(OUTPUT_FB, "  top    - monitor de processos em tempo real\n");
    kprintf(OUTPUT_FB, "  info   - informacoes do sistema\n");
    kprintf(OUTPUT_FB, "  reboot - reinicia o sistema\n");
}

static void cmd_ps(void)
{
    unsigned int i;
    kprintf(OUTPUT_FB, "PID  NOME             ESTADO   TICKS\n");
    kprintf(OUTPUT_FB, "---  ---------------  -------  -----\n");
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &process_table[i];
        if (p->state == PROC_UNUSED)
            continue;
        unsigned int row = fb_get_cursor_row();
        fb_clear_line(row);
        kprintf(OUTPUT_FB, "%d", p->pid);
        fb_write_str_at(row, 5,  p->name,              FB_LIGHT_GREY, FB_BLACK);
        fb_write_str_at(row, 21, state_name(p->state), FB_LIGHT_GREY, FB_BLACK);
        fb_set_cursor(row, 29);
        kprintf(OUTPUT_FB, "%d\n", p->ticks_total);
    }
}

static void cmd_info(void)
{
    kprintf(OUTPUT_FB, "Sistema Operacional - Projeto SO\n");
    kprintf(OUTPUT_FB, "Uptime: %d ticks (%d ms)\n",
            pit_get_ticks(), pit_get_ticks() * 10);
}

static void cmd_reboot(void)
{
    kprintf(OUTPUT_FB, "Reiniciando...\n");
    /* Pulso no controlador de teclado 8042 para reset */
    outb(0x64, 0xFE);
    /* Fallback: triple fault */
    asm volatile("lidt 0");
    asm volatile("int $0");
}

static void shell_execute(const char *cmd)
{
    if (strcmp(cmd, "help") == 0)       cmd_help();
    else if (strcmp(cmd, "clear") == 0) fb_clear();
    else if (strcmp(cmd, "ps") == 0)    cmd_ps();
    else if (strcmp(cmd, "top") == 0)   top_run();
    else if (strcmp(cmd, "info") == 0)  cmd_info();
    else if (strcmp(cmd, "reboot") == 0) cmd_reboot();
    else if (strlen(cmd) > 0)
        kprintf(OUTPUT_FB, "comando desconhecido: %s\n", cmd);
}

void shell_run(void)
{
    char buf[128];
    kprintf(OUTPUT_FB, "\nBem-vindo ao Mini-Shell! Digite 'help' para ajuda.\n\n");
    while (1) {
        kprintf(OUTPUT_FB, "kernel> ");
        kbd_readline(buf, sizeof(buf));
        shell_execute(buf);
    }
}
