#include "shell.h"
#include "keyboard.h"
#include "fb.h"
#include "printf.h"
#include "string.h"
#include "process.h"
#include "pit.h"
#include "top.h"
#include "scheduler.h"
#include "serial.h"

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

/* Processo de teste: loop de CPU puro, cede a cada 50000 iteracoes */
static void worker_proc(void)
{
    volatile unsigned int counter = 0;
    kprintf(OUTPUT_SERIAL, "[worker] iniciou pid=%d\n",
        current_process ? current_process->pid : 99);
    while (1) {
        counter++;
        if (counter % 50000 == 0) {
            kprintf(OUTPUT_SERIAL, "[worker] yield counter=%d ticks=%d\n",
                counter, pit_get_ticks());
            yield();
        }
    }
}

static void cmd_spawn(const char *name)
{
    process_t *p = process_create_kernel(
        (name && name[0]) ? name : "worker", worker_proc);
    if (p)
        kprintf(OUTPUT_FB, "Processo criado: PID %d (%s)\n", p->pid, p->name);
    else
        kprintf(OUTPUT_FB, "Erro: tabela de processos cheia\n");
}

static void cmd_help(void)
{
    kprintf(OUTPUT_FB, "Comandos disponiveis:\n");
    kprintf(OUTPUT_FB, "  help   - lista os comandos\n");
    kprintf(OUTPUT_FB, "  clear  - limpa a tela\n");
    kprintf(OUTPUT_FB, "  ps     - lista processos\n");
    kprintf(OUTPUT_FB, "  top    - monitor de processos em tempo real\n");
    kprintf(OUTPUT_FB, "  info   - informacoes do sistema\n");
    kprintf(OUTPUT_FB, "  spawn  - cria processo de teste (spawn [nome])\n");
    kprintf(OUTPUT_FB, "  kill   - mata um processo (kill <pid>)\n");
    kprintf(OUTPUT_FB, "  reboot   - reinicia o sistema\n");
    kprintf(OUTPUT_FB, "  poweroff  - desliga o sistema\n");
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
    outb(0x64, 0xFE);
    /* fallback: triple fault */
    asm volatile("lidt 0");
    asm volatile("int $0");
}

static void cmd_poweroff(void)
{
    kprintf(OUTPUT_FB, "Desligando...\n");
    /* Bochs: escreve "Shutdown" na porta 0x8900 */
    const char *s = "Shutdown";
    while (*s)
        outb(0x8900, (unsigned char)*s++);
    /* QEMU/KVM: porta 0x604 */
    outb(0x604, 0x00);
    outb(0x604, 0x20);
    /* fallback: halt */
    asm volatile("cli");
    while (1) asm volatile("hlt");
}

static void shell_execute(const char *cmd)
{
    if (strcmp(cmd, "help") == 0)       cmd_help();
    else if (strcmp(cmd, "clear") == 0) fb_clear();
    else if (strcmp(cmd, "ps") == 0)    cmd_ps();
    else if (strcmp(cmd, "top") == 0)   top_run();
    else if (strcmp(cmd, "info") == 0)  cmd_info();
    else if (strncmp(cmd, "spawn", 5) == 0) {
        const char *arg = (strlen(cmd) > 6) ? cmd + 6 : "";
        cmd_spawn(arg);
    }
    else if (strncmp(cmd, "kill", 4) == 0) {
        if (strlen(cmd) > 5) {
            unsigned int pid = (unsigned int)atoi(cmd + 5);
            if (process_kill(pid) == 0)
                kprintf(OUTPUT_FB, "Processo %d encerrado\n", pid);
            else
                kprintf(OUTPUT_FB, "kill: PID %d invalido\n", pid);
        } else {
            kprintf(OUTPUT_FB, "uso: kill <pid>\n");
        }
    }
    else if (strcmp(cmd, "reboot") == 0)   cmd_reboot();
    else if (strcmp(cmd, "poweroff") == 0) cmd_poweroff();
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
