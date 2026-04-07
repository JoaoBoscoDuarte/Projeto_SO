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

/* =========================================================
 * [NOVO] Configuracoes do historico do shell
 * ---------------------------------------------------------
 * SHELL_MAX_LINE:
 *   tamanho maximo da linha que o shell edita localmente.
 *
 * SHELL_HISTORY_SIZE:
 *   quantidade maxima de comandos guardados no historico.
 * =========================================================
 */
#define SHELL_MAX_LINE     128
#define SHELL_HISTORY_SIZE 16

/* =========================================================
 * [NOVO] Buffer de historico
 * ---------------------------------------------------------
 * shell_history:
 *   guarda os ultimos comandos digitados.
 *
 * shell_history_count:
 *   quantidade atual de comandos validos armazenados.
 * =========================================================
 */
static char shell_history[SHELL_HISTORY_SIZE][SHELL_MAX_LINE];
static unsigned int shell_history_count = 0;


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
/*
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
*/
// processo de teste mais simples, apenas para debugg.
static void worker_proc(void)
{
    volatile unsigned int counter = 0;

    while (1) {
        counter++;

        /* apenas cede a CPU periodicamente */
        if (counter % 50000 == 0)
            yield();
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

/* =========================================================
 * [NOVO] Salva um comando no historico
 * ---------------------------------------------------------
 * Regras:
 * - nao salva linha vazia
 * - nao duplica o ultimo comando imediatamente anterior
 * - se o historico encher, descarta o mais antigo
 * =========================================================
 */
static void shell_history_push(const char *cmd)
{
    unsigned int i;

    if (!cmd || cmd[0] == '\0')
        return;

    /* evita repetir o ultimo comando duas vezes seguidas */
    if (shell_history_count > 0 &&
        strcmp(shell_history[shell_history_count - 1], cmd) == 0)
        return;

    /* se ainda ha espaco, adiciona no final */
    if (shell_history_count < SHELL_HISTORY_SIZE) {
        strcpy(shell_history[shell_history_count], cmd);
        shell_history_count++;
        return;
    }

    /* se encheu, desloca tudo uma posicao para cima */
    for (i = 1; i < SHELL_HISTORY_SIZE; i++)
        strcpy(shell_history[i - 1], shell_history[i]);

    strcpy(shell_history[SHELL_HISTORY_SIZE - 1], cmd);
}

/* =========================================================
 * [NOVO] Reescreve a linha atual do shell na tela
 * ---------------------------------------------------------
 * Essa funcao eh usada quando o usuario aperta seta para
 * cima/baixo e o shell precisa trocar o comando atual pelo
 * comando do historico.
 *
 * row:
 *   linha da tela onde o comando esta sendo editado
 *
 * start_col:
 *   coluna onde comeca a area digitavel (logo apos "kernel> ")
 *
 * buf:
 *   conteudo novo a ser mostrado
 *
 * old_len:
 *   tamanho do comando antigo, para conseguir apagar sobras
 * =========================================================
 */
static void shell_redraw_line(
    unsigned int row,
    unsigned int start_col,
    const char *buf,
    unsigned int old_len)
{
    unsigned int i;
    unsigned int new_len = strlen(buf);

    /* apaga o conteudo antigo */
    for (i = 0; i < old_len; i++) {
        fb_write_at(row, start_col + i, ' ', FB_LIGHT_GREY, FB_BLACK);
    }

    /* escreve o novo conteudo */
    for (i = 0; i < new_len; i++) {
        fb_write_at(row, start_col + i, buf[i], FB_LIGHT_GREY, FB_BLACK);
    }

    /* posiciona o cursor no fim do texto novo */
    fb_set_cursor(row, start_col + new_len);
}

/* =========================================================
 * [NOVO] Leitura de linha do shell com suporte a historico
 * ---------------------------------------------------------
 * Esta funcao substitui o uso direto de kbd_readline().
 *
 * Ela continua tratando:
 * - caracteres comuns
 * - backspace
 * - enter
 *
 * E agora tambem trata:
 * - ESC [ A  -> seta para cima
 * - ESC [ B  -> seta para baixo
 *
 * Observacao:
 * isso depende do driver de teclado transformar as setas
 * nessas sequencias.
 * =========================================================
 */
static int shell_readline(char *buf, unsigned int max_len)
{
    unsigned int i = 0;

    /* guarda a posicao em que o usuario comeca a digitar */
    unsigned int row = fb_get_cursor_row();
    unsigned int start_col = fb_get_cursor_col();

    /* old_len serve para apagar completamente a linha anterior
       quando navegarmos pelo historico */
    unsigned int old_len = 0;

    /* hist_index:
     * - começa "apos o fim" do historico
     * - quando sobe, vai para comandos antigos
     * - quando desce, volta para comandos mais novos
     */
    int hist_index = (int)shell_history_count;

    /* current_edit guarda o que o usuario estava digitando antes
       de entrar na navegacao do historico */
    char current_edit[SHELL_MAX_LINE];

    if (!buf || max_len == 0)
        return 0;

    buf[0] = '\0';
    current_edit[0] = '\0';

    while (1) {
        char c = kbd_getchar();

        /* ENTER finaliza a linha */
        if (c == '\n') {
            buf[i] = '\0';
            fb_putchar('\n');
            return (int)i;
        }

        /* BACKSPACE apaga um caractere, se existir */
        if (c == '\b') {
            if (i > 0) {
                i--;
                buf[i] = '\0';
                fb_putchar('\b');
                old_len = i;
            }
            continue;
        }

        /* =====================================================
         * [NOVO] Tratamento das sequencias de escape das setas
         * -----------------------------------------------------
         * ESC [ A = seta para cima
         * ESC [ B = seta para baixo
         * =====================================================
         */
        if (c == 27) { /* ESC */
            char c1 = kbd_getchar();
            char c2 = kbd_getchar();

            if (c1 == '[' && c2 == 'A') {
                /* ---------- SETA PARA CIMA ----------
                 * Vai para um comando mais antigo.
                 */

                if (shell_history_count > 0) {
                    /* ao entrar no historico pela primeira vez,
                       salvamos o que o usuario estava digitando */
                    if (hist_index == (int)shell_history_count)
                        strcpy(current_edit, buf);

                    if (hist_index > 0)
                        hist_index--;

                    strcpy(buf, shell_history[hist_index]);
                    i = strlen(buf);

                    shell_redraw_line(row, start_col, buf, old_len);
                    old_len = i;
                }
                continue;
            }

            if (c1 == '[' && c2 == 'B') {
                /* ---------- SETA PARA BAIXO ----------
                 * Vai para um comando mais novo.
                 */

                if (hist_index < (int)shell_history_count - 1) {
                    hist_index++;
                    strcpy(buf, shell_history[hist_index]);
                } else if (hist_index == (int)shell_history_count - 1) {
                    /* se estava no ultimo item do historico e descer,
                       volta para o texto que o usuario estava editando */
                    hist_index = (int)shell_history_count;
                    strcpy(buf, current_edit);
                } else {
                    /* ja esta fora do historico; nao faz nada */
                    continue;
                }

                i = strlen(buf);

                shell_redraw_line(row, start_col, buf, old_len);
                old_len = i;
                continue;
            }

            /* qualquer outra sequencia ESC eh ignorada */
            continue;
        }

        /* caractere comum */
        if (i < max_len - 1) {
            buf[i++] = c;
            buf[i] = '\0';
            fb_putchar(c);
            old_len = i;
        }
    }
}

void shell_run(void)
{
    char buf[128];

    kprintf(OUTPUT_FB, "\nBem-vindo ao Mini-Shell! Digite 'help' para ajuda.\n\n");

    while (1) {
        kprintf(OUTPUT_FB, "kernel> ");

        /* =====================================================
         * [ALTERADO]
         * Antes:
         *   kbd_readline(buf, sizeof(buf));
         *
         * Agora:
         *   shell_readline(buf, sizeof(buf));
         *
         * Motivo:
         *   agora quem controla a leitura da linha eh o shell,
         *   para conseguir tratar historico e setas.
         * =====================================================
         */
        shell_readline(buf, sizeof(buf));

        /* =====================================================
         * [NOVO]
         * Salva o comando no historico antes de executar.
         * Nao salva linha vazia e nao duplica o ultimo.
         * =====================================================
         */
        shell_history_push(buf);

        shell_execute(buf);
    }
}
