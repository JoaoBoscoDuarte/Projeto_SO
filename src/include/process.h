#ifndef INCLUDE_PROCESS_H
#define INCLUDE_PROCESS_H

/* ============================================================================
 * process.h — Process Control Block (PCB) e tabela de processos
 *
 * Suporta até MAX_PROCESSES processos simultâneos.
 * PID 0 = kernel/idle (nunca alocado por process_create_full).
 * ========================================================================== */

#define MAX_PROCESSES  16
#define PROC_NAME_LEN  32

/* Estado de um processo */
typedef enum {
    PROC_UNUSED  = 0,   /* slot vazio na tabela */
    PROC_READY,         /* pronto para executar */
    PROC_RUNNING,       /* em execução (somente um por vez) */
    PROC_BLOCKED,       /* aguardando E/S ou evento */
    PROC_ZOMBIE         /* terminou, aguardando coleta */
} proc_state_t;

/* Process Control Block */
typedef struct process {
    unsigned int   pid;
    char           name[PROC_NAME_LEN];
    proc_state_t   state;

    /* Registradores salvos para context switch (kernel stack frame) */
    unsigned int   esp;         /* kernel stack pointer salvo          */
    unsigned int   ebp;
    unsigned int   eip;         /* ponto de entrada / resume point     */

    /* Memória */
    unsigned int   page_directory_phys; /* endereço físico do PD      */
    unsigned int   kernel_stack_base;   /* base (endereço baixo) da kstack */
    unsigned int   kernel_stack_top;    /* topo (esp0 no TSS)          */

    /* Contagem de tempo */
    unsigned int   ticks_total;         /* ticks de CPU consumidos         */

    /* Memória alocada */
    unsigned int   mem_frames;          /* frames físicos alocados         */

    /* Estado user-space salvo (para retorno ao ring 3) */
    unsigned int   user_eip;
    unsigned int   user_esp;
} process_t;

/* ============================================================================
 * Variáveis globais exportadas
 * ========================================================================== */

/* Tabela estática de todos os processos */
extern process_t  process_table[MAX_PROCESSES];

/* Ponteiro para o processo atualmente em execução */
extern process_t *current_process;

/* ============================================================================
 * API de gerenciamento de processos
 * ========================================================================== */

/*
 * process_init — inicializa a tabela de processos e registra o PID 0 (kernel).
 * Deve ser chamada uma única vez em kmain(), antes de criar qualquer processo.
 */
void process_init(void);

/*
 * process_create_full — cria um processo user-mode completo.
 *
 *   name      : nome do processo (copiado, truncado em PROC_NAME_LEN-1)
 *   code_phys : endereço físico do binário flat já carregado pelo GRUB
 *   code_size : tamanho em bytes do binário
 *
 * Aloca:
 *   - Page directory próprio (cópia das entradas do kernel)
 *   - Frame(s) para o código user
 *   - Frame para a stack user  (mapeado em 0xBFFFF000)
 *   - Frame para a kernel stack do processo
 *
 * Retorna ponteiro para o process_t na tabela, ou NULL em caso de falha.
 */
process_t *process_create_full(const char *name,
                               unsigned int code_phys,
                               unsigned int code_size);

/*
 * process_create_kernel — cria um processo que roda uma função C em ring 0.
 *
 *   name : nome do processo
 *   func : ponteiro para a função de entrada (void func(void))
 *
 * Aloca uma kernel stack, monta o frame inicial que context_switch espera
 * encontrar, e insere o processo como READY na tabela.
 *
 * Retorna ponteiro para o process_t, ou NULL em falha.
 */
process_t *process_create_kernel(const char *name, void (*func)(void));

/*
 * process_kill — marca um processo como ZOMBIE pelo PID.
 * Retorna 0 em sucesso, -1 se o PID não existir ou for o kernel (PID 0).
 */
int process_kill(unsigned int pid);

/*
 * process_exit — marca o processo corrente como ZOMBIE.
 * O scheduler deve liberar recursos posteriormente.
 */
void process_exit(void);

/*
 * process_get_by_pid — busca na tabela pelo PID.
 * Retorna NULL se não encontrado ou slot UNUSED.
 */
process_t *process_get_by_pid(unsigned int pid);

/*
 * process_next_pid — retorna o próximo PID disponível (≥ 1).
 * Retorna 0 se a tabela estiver cheia.
 */
unsigned int process_next_pid(void);

/* ============================================================================
 * Compatibilidade com código legado do Capítulo 11
 *
 * process_create() original ainda é usada em kmain.c.
 * Mantida aqui para não quebrar a compilação.
 * ========================================================================== */
struct process_legacy {
    unsigned int page_directory_phys;
    unsigned int eip;
    unsigned int esp;
};

struct process_legacy process_create(unsigned int code_phys,
                                     unsigned int code_size);

/*
 * enter_usermode — transição ring 0 → ring 3 via iret (usermode.s).
 * Não retorna.
 */
extern void enter_usermode(unsigned int eip,
                           unsigned int esp,
                           unsigned int page_directory_phys);

#endif /* INCLUDE_PROCESS_H */
