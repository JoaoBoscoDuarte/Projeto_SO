#include "process.h"
#include "paging.h"
#include "pfa.h"
#include "scheduler.h"

/* page_directory definido em loader.s — PD do kernel ativo em CR3 */
extern pde_t page_directory[1024];

/* ============================================================================
 * process.c — Criação e gerenciamento de processos
 *
 * Layout do espaço virtual de cada processo user-mode:
 *
 *   0x00000000 – 0x00000FFF  : código user   (1 página, PAGE_USER_RW)
 *   0xBFFFF000 – 0xBFFFFFFF  : stack user    (1 página, PAGE_USER_RW)
 *   0xC0000000 – 0xC03FFFFF  : kernel        (copiado do PD atual)
 *   0xC2000000 + pid*0x2000  : kernel stack  (1 página, PAGE_KERNEL)
 *                              (região dedicada por processo)
 * ========================================================================== */

/* Endereços virtuais do espaço user */
#define USER_CODE_VADDR   0x00000000u
#define USER_STACK_PAGE   0xBFFFF000u
#define USER_STACK_TOP    (USER_STACK_PAGE + PAGE_SIZE - 4u)

/*
 * Região de kernel stacks por processo.
 * PID 1 → 0xC2002000, PID 2 → 0xC2004000, …
 * Stride de 0x2000 (8 KB) para dar espaço de guard page entre stacks.
 */
#define KSTACK_BASE       0xC2000000u
#define KSTACK_STRIDE     0x2000u
#define KSTACK_SIZE       PAGE_SIZE   /* 4 KB por processo */

/* ============================================================================
 * Tabela global de processos e ponteiro para o processo corrente
 * ========================================================================== */
process_t  process_table[MAX_PROCESSES];
process_t *current_process = (process_t *)0;

/* ============================================================================
 * Helpers internos — acesso a page directories de outros processos via temp_map
 * ========================================================================== */

static int proc_map_page(unsigned int pd_phys,
                         unsigned int virt,
                         unsigned int phys,
                         unsigned int flags)
{
    unsigned int pd_idx = PD_INDEX(virt);
    unsigned int pt_idx = PT_INDEX(virt);
    pde_t *pd;
    pte_t *pt;
    unsigned int pt_phys;
    unsigned int i;
    pde_t pde;

    pd  = (pde_t *)temp_map(pd_phys);
    pde = pd[pd_idx];
    temp_unmap();

    if (pde & PAGE_PRESENT) {
        pt_phys = PDE_FRAME(pde);
    } else {
        pt_phys = pfa_alloc();
        if (pt_phys == 0)
            return -1;

        pt = (pte_t *)temp_map(pt_phys);
        for (i = 0; i < 1024; i++)
            pt[i] = 0;
        temp_unmap();

        pd = (pde_t *)temp_map(pd_phys);
        pd[pd_idx] = pt_phys | PAGE_USER_RW;
        temp_unmap();
    }

    pt = (pte_t *)temp_map(pt_phys);
    pt[pt_idx] = (phys & 0xFFFFF000u) | (flags | PAGE_PRESENT);
    temp_unmap();

    return 0;
}

/* Zera n bytes em dst */
static void mem_zero(unsigned char *dst, unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        dst[i] = 0;
}

/* Copia string com limite */
static void str_copy_n(char *dst, const char *src, unsigned int max)
{
    unsigned int i;
    for (i = 0; i + 1 < max && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

/* ============================================================================
 * process_init — inicializa a tabela e registra o kernel como PID 0
 * ========================================================================== */
void process_init(void)
{
    unsigned int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid   = 0;
        process_table[i].state = PROC_UNUSED;
    }

    /* PID 0 = kernel/idle */
    process_table[0].pid         = 0;
    process_table[0].state       = PROC_RUNNING;
    process_table[0].ticks_total = 0;
    process_table[0].mem_frames  = 0;
    str_copy_n(process_table[0].name, "kernel", PROC_NAME_LEN);
    current_process = &process_table[0];
}

/* ============================================================================
 * process_next_pid — retorna o próximo PID livre (slot UNUSED), ou 0 se cheio
 * ========================================================================== */
unsigned int process_next_pid(void)
{
    unsigned int i;
    for (i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED)
            return i;
    }
    return 0;
}

/* ============================================================================
 * process_get_by_pid
 * ========================================================================== */
process_t *process_get_by_pid(unsigned int pid)
{
    unsigned int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_UNUSED &&
            process_table[i].pid == pid)
            return &process_table[i];
    }
    return (process_t *)0;
}

/* ============================================================================
 * process_kill — mata um processo pelo PID
 * ========================================================================== */
int process_kill(unsigned int pid)
{
    process_t *p;

    if (pid == 0)
        return -1; /* não mata o kernel */

    p = process_get_by_pid(pid);
    if (!p)
        return -1;

    p->state = PROC_ZOMBIE;

    /* Se matou o processo atual, cede a CPU imediatamente */
    if (p == current_process)
        yield();

    return 0;
}

/* ============================================================================
 * process_exit — marca o processo corrente como ZOMBIE
 * ========================================================================== */
void process_exit(void)
{
    if (current_process)
        current_process->state = PROC_ZOMBIE;
}

/* ============================================================================
 * process_create_kernel — cria processo ring 0 a partir de uma função C
 *
 * Layout do frame inicial na kernel stack (lido por context_switch):
 *
 *   [topo da stack - 4]  endereço de retorno = func  ← ret em context_switch
 *   [topo da stack - 8]  edi = 0
 *   [topo da stack - 12] esi = 0
 *   [topo da stack - 16] ebx = 0
 *   [topo da stack - 20] ebp = 0  ← esp salvo em proc->esp
 *
 * context_switch faz: pop edi/esi/ebx/ebp, ret → salta para func.
 * ========================================================================== */
process_t *process_create_kernel(const char *name, void (*func)(void))
{
    unsigned int pid;
    unsigned int kstack_frame;
    unsigned int kstack_virt;
    unsigned int *stack;
    process_t *proc;

    pid = process_next_pid();
    if (pid == 0)
        return (process_t *)0;

    proc = &process_table[pid];

    /* Aloca e mapeia a kernel stack */
    kstack_frame = pfa_alloc();
    if (kstack_frame == 0)
        return (process_t *)0;

    kstack_virt = KSTACK_BASE + pid * KSTACK_STRIDE;
    if (paging_map(kstack_virt, kstack_frame, PAGE_KERNEL) != 0) {
        pfa_free(kstack_frame);
        return (process_t *)0;
    }

    /* Zera a stack */
    stack = (unsigned int *)kstack_virt;
    {
        unsigned int i;
        for (i = 0; i < KSTACK_SIZE / sizeof(unsigned int); i++)
            stack[i] = 0;
    }

    /* Monta o frame inicial no topo da stack:
     * context_switch faz 4x pop (edi,esi,ebx,ebp) depois ret.
     * Empilhamos de cima para baixo: ret_addr, edi, esi, ebx, ebp */
    unsigned int top = kstack_virt + KSTACK_SIZE;
    unsigned int *sp = (unsigned int *)top;

    /* context_switch faz (em ordem): pop edi, pop esi, pop ebx, pop ebp, ret
     * Portanto empilhamos de cima para baixo (ordem inversa dos pops):
     *   ret_addr  ← popado por ret
     *   ebp = 0   ← popado por pop ebp
     *   ebx = 0   ← popado por pop ebx
     *   esi = 0   ← popado por pop esi
     *   edi = 0   ← popado por pop edi  ← ESP aponta aqui
     */
    *(--sp) = (unsigned int)func; /* ret addr */
    *(--sp) = 0;                  /* ebp */
    *(--sp) = 0;                  /* ebx */
    *(--sp) = 0;                  /* esi */
    *(--sp) = 0;                  /* edi */ /* proc->esp aponta aqui */

    proc->pid                 = pid;
    proc->state               = PROC_READY;
    proc->ticks_total         = 0;
    proc->mem_frames          = 1;  /* 1 frame de kernel stack */
    proc->kernel_stack_base   = kstack_virt;
    proc->kernel_stack_top    = kstack_virt + KSTACK_SIZE;
    proc->esp                 = (unsigned int)sp;
    proc->ebp                 = (unsigned int)sp;
    proc->eip                 = (unsigned int)func;
    proc->page_directory_phys = 0; /* usa o PD do kernel */
    proc->user_eip            = 0;
    proc->user_esp            = 0;
    str_copy_n(proc->name, name ? name : "kproc", PROC_NAME_LEN);

    return proc;
}

/* ============================================================================
 * process_create_full — cria um processo user-mode completo com PCB
 *
 * Passos:
 *   1. Reserva slot na tabela de processos
 *   2. Aloca e inicializa page directory
 *   3. Copia entradas do kernel (índices 768+)
 *   4. Aloca frame para o código user e copia o binário
 *   5. Mapeia o código em 0x00000000 (PAGE_USER_RW)
 *   6. Aloca frame para a stack user e mapeia em 0xBFFFF000
 *   7. Aloca kernel stack e mapeia na região dedicada
 *   8. Preenche o PCB e retorna
 * ========================================================================== */
process_t *process_create_full(const char *name,
                               unsigned int code_phys,
                               unsigned int code_size)
{
    unsigned int pid;
    unsigned int pd_phys;
    unsigned int code_frame;
    unsigned int stack_frame;
    unsigned int kstack_frame;
    unsigned int kstack_virt;
    unsigned int i;
    pde_t *pd;
    pde_t *kernel_pd;
    unsigned char *src;
    unsigned char *dst;
    process_t *proc;

    /* Passo 1: reserva slot */
    pid = process_next_pid();
    if (pid == 0)
        return (process_t *)0;

    proc = &process_table[pid];

    /* Passo 2: aloca e zera o page directory do processo */
    pd_phys = pfa_alloc();
    if (pd_phys == 0)
        return (process_t *)0;

    pd = (pde_t *)temp_map(pd_phys);
    for (i = 0; i < 1024; i++)
        pd[i] = 0;
    temp_unmap();

    /* Passo 3: copia entradas do kernel (0xC0000000+) para o PD do processo */
    pd = (pde_t *)temp_map(pd_phys);
    kernel_pd = page_directory;
    for (i = 768; i < 1024; i++)
        pd[i] = kernel_pd[i];
    temp_unmap();

    /* Passo 4: aloca frame para o código user e copia o binário */
    code_frame = pfa_alloc();
    if (code_frame == 0) {
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    src = (unsigned char *)(code_phys + KERNEL_VIRT_BASE);
    dst = (unsigned char *)temp_map(code_frame);
    for (i = 0; i < PAGE_SIZE; i++) {
        if (i < code_size)
            dst[i] = src[i];
        else
            dst[i] = 0;
    }
    temp_unmap();

    /* Passo 5: mapeia código em virtual 0x00000000 */
    if (proc_map_page(pd_phys, USER_CODE_VADDR, code_frame, PAGE_USER_RW) != 0) {
        pfa_free(code_frame);
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    /* Passo 6: aloca e mapeia stack user em 0xBFFFF000 */
    stack_frame = pfa_alloc();
    if (stack_frame == 0) {
        pfa_free(code_frame);
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    dst = (unsigned char *)temp_map(stack_frame);
    mem_zero(dst, PAGE_SIZE);
    temp_unmap();

    if (proc_map_page(pd_phys, USER_STACK_PAGE, stack_frame, PAGE_USER_RW) != 0) {
        pfa_free(stack_frame);
        pfa_free(code_frame);
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    /* Passo 7: aloca kernel stack e mapeia na região dedicada */
    kstack_frame = pfa_alloc();
    if (kstack_frame == 0) {
        pfa_free(stack_frame);
        pfa_free(code_frame);
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    /* Zera a kernel stack para segurança */
    dst = (unsigned char *)temp_map(kstack_frame);
    mem_zero(dst, PAGE_SIZE);
    temp_unmap();

    /*
     * Endereço virtual da kernel stack no espaço do kernel.
     * Como está na região do kernel (>= 0xC0000000), é compartilhada por todos
     * os PDs (entrada copiada do kernel PD em paging_map).
     */
    kstack_virt = KSTACK_BASE + pid * KSTACK_STRIDE;
    if (paging_map(kstack_virt, kstack_frame, PAGE_KERNEL) != 0) {
        pfa_free(kstack_frame);
        pfa_free(stack_frame);
        pfa_free(code_frame);
        pfa_free(pd_phys);
        return (process_t *)0;
    }

    /* Passo 8: preenche o PCB */
    proc->pid                 = pid;
    proc->state               = PROC_READY;
    proc->ticks_total         = 0;
    proc->mem_frames          = 4;  /* pd + code + stack_user + kstack */
    str_copy_n(proc->name, name ? name : "proc", PROC_NAME_LEN);
    proc->page_directory_phys = pd_phys;
    proc->kernel_stack_base   = kstack_virt;
    proc->kernel_stack_top    = kstack_virt + KSTACK_SIZE;
    proc->user_eip            = USER_CODE_VADDR;
    proc->user_esp            = USER_STACK_TOP;
    proc->eip                 = USER_CODE_VADDR;
    proc->esp                 = proc->kernel_stack_top;
    proc->ebp                 = proc->kernel_stack_top;

    return proc;
}

/* ============================================================================
 * process_create — API legada do Capítulo 11 (mantida para kmain.c)
 * ========================================================================== */
struct process_legacy process_create(unsigned int code_phys,
                                     unsigned int code_size)
{
    struct process_legacy legacy;
    process_t *proc;

    legacy.page_directory_phys = 0;
    legacy.eip = 0;
    legacy.esp = 0;

    proc = process_create_full("init", code_phys, code_size);
    if (!proc)
        return legacy;

    legacy.page_directory_phys = proc->page_directory_phys;
    legacy.eip                 = proc->user_eip;
    legacy.esp                 = proc->user_esp;
    return legacy;
}
