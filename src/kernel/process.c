#include "process.h"
#include "paging.h"
#include "pfa.h"

/* page_directory definido em loader.s (.data) — PD do kernel ativo em CR3 */
extern pde_t page_directory[1024];

/*
 * process.c — Criação de processos user mode
 *
 * Layout do espaço de endereçamento virtual do processo:
 *
 *   0x00000000 – 0x00000FFF  : código do user (1 página, PAGE_USER_RW)
 *   0xBFFFF000 – 0xBFFFFFFF  : stack do user  (1 página, PAGE_USER_RW)
 *   0xC0000000 – 0xC03FFFFF  : kernel (copiado do PD atual, PAGE_KERNEL)
 *                              → inacessível em ring 3 (U/S=0)
 *
 * Virtual address do entry point : 0x00000000
 * Virtual address do topo da stack: 0xBFFFFFFB (topo da última página − 4)
 */

/* Endereço virtual onde o código do processo será mapeado */
#define USER_CODE_VADDR  0x00000000u

/* Endereço virtual da página de stack do processo */
#define USER_STACK_PAGE  0xBFFFF000u

/* Topo da stack: última posição usável da página (alinhada em 4 bytes) */
#define USER_STACK_TOP   (USER_STACK_PAGE + PAGE_SIZE - 4u)

/* =========================================================================
 * Helpers internos para escrever em page directories/tables de um processo
 *
 * Como o page directory do processo NÃO é o CR3 ativo, não podemos acessá-lo
 * diretamente. Usamos temp_map() para mapear temporariamente o frame físico
 * no endereço virtual fixo 0xC07FF000 e escrever os dados necessários.
 * ========================================================================= */

/*
 * proc_map_page — mapeia uma página no page directory do processo
 *
 * pd_phys    : endereço físico do page directory do processo
 * virt       : endereço virtual a mapear (alinhado em PAGE_SIZE)
 * phys       : endereço físico do frame (alinhado em PAGE_SIZE)
 * flags      : flags de página (ex.: PAGE_USER_RW)
 *
 * Retorna 0 em sucesso, -1 em falha de alocação.
 */
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

    /* Mapeia o PD do processo temporariamente para leitura/escrita */
    pd = (pde_t *)temp_map(pd_phys);
    pde = pd[pd_idx];
    temp_unmap();

    if (pde & PAGE_PRESENT) {
        /* Page table já existe — obtém seu endereço físico */
        pt_phys = PDE_FRAME(pde);
    } else {
        /* Aloca um novo frame para a page table */
        pt_phys = pfa_alloc();
        if (pt_phys == 0)
            return -1;

        /* Zera a nova page table */
        pt = (pte_t *)temp_map(pt_phys);
        for (i = 0; i < 1024; i++)
            pt[i] = 0;
        temp_unmap();

        /* Instala a nova PT no PD do processo.
         * Usamos PAGE_USER | PAGE_WRITE | PAGE_PRESENT para que o hardware
         * permita que a PT cubra tanto páginas kernel quanto user. */
        pd = (pde_t *)temp_map(pd_phys);
        pd[pd_idx] = pt_phys | PAGE_USER_RW;
        temp_unmap();
    }

    /* Escreve a entrada na page table do processo */
    pt = (pte_t *)temp_map(pt_phys);
    pt[pt_idx] = (phys & 0xFFFFF000u) | (flags | PAGE_PRESENT);
    temp_unmap();

    return 0;
}

/* =========================================================================
 * process_create(code_phys, code_size)
 * ========================================================================= */
struct process process_create(unsigned int code_phys, unsigned int code_size)
{
    struct process proc;
    unsigned int pd_phys;
    unsigned int code_frame;
    unsigned int stack_frame;
    unsigned int i;
    pde_t *pd;
    pde_t *kernel_pd;
    unsigned char *src;
    unsigned char *dst;

    /* Inicializa com valores de erro */
    proc.page_directory_phys = 0;
    proc.eip = 0;
    proc.esp = 0;

    /* ------------------------------------------------------------------
     * Passo 1: Aloca e inicializa o page directory do processo
     * ------------------------------------------------------------------ */
    pd_phys = pfa_alloc();
    if (pd_phys == 0)
        return proc;

    /* Zera todo o PD do processo */
    pd = (pde_t *)temp_map(pd_phys);
    for (i = 0; i < 1024; i++)
        pd[i] = 0;
    temp_unmap();

    /* ------------------------------------------------------------------
     * Passo 2: Copia as entradas do kernel (índices 768+) do PD atual
     *          para o PD do processo.
     *
     * Isso garante que quando o CPU receber uma interrupção enquanto
     * em ring 3, o kernel continue acessível após trocar para ring 0
     * (sem precisar trocar o CR3 de volta).
     *
     * Nota: page_directory é o símbolo virtual do PD atual, definido
     * em loader.s e acessível por extern em paging.c.
     * ------------------------------------------------------------------ */
    pd = (pde_t *)temp_map(pd_phys);
    kernel_pd = page_directory;   /* PD do kernel — acesso via VMA diretamente */
    for (i = 768; i < 1024; i++)
        pd[i] = kernel_pd[i];
    temp_unmap();

    /* ------------------------------------------------------------------
     * Passo 3: Aloca frame para o código, copia o binário
     * ------------------------------------------------------------------ */
    code_frame = pfa_alloc();
    if (code_frame == 0)
        return proc;

    /* Copia o código do módulo para o frame alocado.
     * code_phys é endereço FÍSICO — converte para virtual para leitura. */
    src = (unsigned char *)(code_phys + KERNEL_VIRT_BASE);
    dst = (unsigned char *)temp_map(code_frame);

    for (i = 0; i < PAGE_SIZE; i++) {
        if (i < code_size)
            dst[i] = src[i];
        else
            dst[i] = 0;   /* zera o restante da página */
    }
    temp_unmap();

    /* ------------------------------------------------------------------
     * Passo 4: Mapeia o código em virtual 0x00000000 no PD do processo
     * ------------------------------------------------------------------ */
    if (proc_map_page(pd_phys, USER_CODE_VADDR, code_frame, PAGE_USER_RW) != 0)
        return proc;

    /* ------------------------------------------------------------------
     * Passo 5: Aloca frame para a stack do user
     * ------------------------------------------------------------------ */
    stack_frame = pfa_alloc();
    if (stack_frame == 0)
        return proc;

    /* Zera a stack */
    dst = (unsigned char *)temp_map(stack_frame);
    for (i = 0; i < PAGE_SIZE; i++)
        dst[i] = 0;
    temp_unmap();

    /* ------------------------------------------------------------------
     * Passo 6: Mapeia a stack em virtual 0xBFFFF000 no PD do processo
     * ------------------------------------------------------------------ */
    if (proc_map_page(pd_phys, USER_STACK_PAGE, stack_frame, PAGE_USER_RW) != 0)
        return proc;

    /* ------------------------------------------------------------------
     * Passo 7: Preenche a struct process e retorna
     * ------------------------------------------------------------------ */
    proc.page_directory_phys = pd_phys;
    proc.eip = USER_CODE_VADDR;
    proc.esp = USER_STACK_TOP;

    return proc;
}
