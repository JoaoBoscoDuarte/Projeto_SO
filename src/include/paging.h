#ifndef PAGING_H
#define PAGING_H

/* ============================================================================
 * paging.h — Subsistema de paginação x86 (4KB pages, two-level page tables)
 *
 * Arquitetura:
 *   - Page Directory: 1024 entradas × 4 bytes = 4096 bytes (uma por PDE)
 *   - Page Table:     1024 entradas × 4 bytes = 4096 bytes (uma por PTE)
 *   - Cada PTE cobre 4KB → uma Page Table cobre 4MB
 *   - O Page Directory cobre 4GB (1024 × 4MB)
 *
 * Layout do espaço virtual do kernel:
 *   0x00000000 – 0xBFFFFFFF  : espaço de usuário (futuro)
 *   0xC0000000 – 0xC03FFFFF  : kernel (entrada 768 do PD, 4MB)
 *   0xC0400000 – 0xC07FFFFF  : região de mapeamento temporário (entrada 769)
 *   0xFFC00000 – 0xFFFFFFFF  : reservado para recursive mapping (futuro)
 *
 * Entrada 769 do Page Directory aponta para uma page table real de 4KB.
 * Usamos a ÚLTIMA entrada dessa page table (índice 1023 → 0xC07FF000)
 * como "janela" para mapear frames físicos temporariamente (Capítulo 10.2).
 * ========================================================================= */

/* --------------------------------------------------------------------------
 * Flags de uma PDE/PTE (bits 0–11)
 * -------------------------------------------------------------------------- */
#define PAGE_PRESENT    (1 << 0)   /* Página está na memória                */
#define PAGE_WRITE      (1 << 1)   /* Leitura/Escrita (0 = somente leitura) */
#define PAGE_USER       (1 << 2)   /* Acessível por user mode (ring 3)      */
#define PAGE_PWT        (1 << 3)   /* Write-Through cache                   */
#define PAGE_PCD        (1 << 4)   /* Cache Disable                         */
#define PAGE_ACCESSED   (1 << 5)   /* CPU seta quando página é acessada     */
#define PAGE_DIRTY      (1 << 6)   /* CPU seta quando página é escrita      */
#define PAGE_PSE        (1 << 7)   /* Page Size Extension (4MB em PDE)      */
#define PAGE_GLOBAL     (1 << 8)   /* Não invalida no flush de TLB          */

/* Flags comuns */
#define PAGE_KERNEL     (PAGE_PRESENT | PAGE_WRITE)
#define PAGE_KERNEL_RO  (PAGE_PRESENT)
#define PAGE_USER_RW    (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

/* --------------------------------------------------------------------------
 * Constantes de layout
 * -------------------------------------------------------------------------- */
#define PAGE_SIZE           0x1000          /* 4096 bytes                    */
#define PAGE_SIZE_4M        0x400000        /* 4MB (PSE)                     */

#define KERNEL_VIRT_BASE    0xC0000000u     /* Base virtual do kernel        */
#define KERNEL_PD_INDEX     768             /* 0xC0000000 >> 22              */

/* Entrada 769: página table de mapeamento temporário                        */
#define TEMP_MAP_PD_INDEX   769
#define TEMP_MAP_BASE       0xC0400000u     /* Base virtual da região temp   */
#define TEMP_MAP_VADDR      0xC07FF000u     /* Última página: slot de temp   */
#define TEMP_MAP_PT_INDEX   1023            /* Índice na page table de temp  */

/* --------------------------------------------------------------------------
 * Macros de conversão de endereços
 * -------------------------------------------------------------------------- */
#define VIRT_TO_PHYS(vaddr) ((unsigned int)(vaddr) - KERNEL_VIRT_BASE)
#define PHYS_TO_VIRT(paddr) ((void *)((unsigned int)(paddr) + KERNEL_VIRT_BASE))

/* Extrai índice no Page Directory (bits 31:22) */
#define PD_INDEX(vaddr)  (((unsigned int)(vaddr)) >> 22)

/* Extrai índice na Page Table (bits 21:12) */
#define PT_INDEX(vaddr)  ((((unsigned int)(vaddr)) >> 12) & 0x3FF)

/* Extrai o endereço físico de um PDE/PTE (alinhado em 4KB, zera bits 0–11) */
#define PDE_FRAME(pde)   ((pde) & 0xFFFFF000u)
#define PTE_FRAME(pte)   ((pte) & 0xFFFFF000u)

/* Alinha um endereço para cima até o próximo limite de página */
#define PAGE_ALIGN_UP(addr) \
    (((unsigned int)(addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* Alinha um endereço para baixo até o limite de página anterior */
#define PAGE_ALIGN_DOWN(addr) \
    ((unsigned int)(addr) & ~(PAGE_SIZE - 1))

/* --------------------------------------------------------------------------
 * Tipos
 * -------------------------------------------------------------------------- */
typedef unsigned int pde_t;   /* Page Directory Entry */
typedef unsigned int pte_t;   /* Page Table Entry     */

/* --------------------------------------------------------------------------
 * API pública
 * -------------------------------------------------------------------------- */

/*
 * paging_init()
 *
 * Inicializa o subsistema de paginação:
 *   1. Cria um Page Directory com page tables reais de 4KB (substitui PSE)
 *   2. Mapeia o kernel em 0xC0000000 com page tables de 4KB
 *   3. Configura a região de mapeamento temporário (entrada 769)
 *   4. Carrega o novo CR3 e habilita paginação (se ainda não estiver ativa)
 *
 * Deve ser chamada UMA vez em kmain(), após pfa_init().
 */
void paging_init(void);

/*
 * paging_map(virt, phys, flags)
 *
 * Mapeia uma página de 4KB:
 *   virt  : endereço virtual (deve estar alinhado em PAGE_SIZE)
 *   phys  : endereço físico  (deve estar alinhado em PAGE_SIZE)
 *   flags : combinação de PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.
 *
 * Se a page table para 'virt' não existir, aloca um novo frame via pfa_alloc()
 * e usa temp_map para inicializá-la com zeros antes de instalar no PD.
 *
 * Retorna 0 em sucesso, -1 em erro (sem frames disponíveis).
 */
int paging_map(unsigned int virt, unsigned int phys, unsigned int flags);

/*
 * paging_unmap(virt)
 *
 * Remove o mapeamento de uma página de 4KB.
 * Invalida a entrada TLB via INVLPG.
 * NÃO libera o frame físico — isso é responsabilidade do chamador via pfa_free().
 */
void paging_unmap(unsigned int virt);

/*
 * paging_get_phys(virt)
 *
 * Retorna o endereço físico mapeado para 'virt', ou 0 se não mapeado.
 */
unsigned int paging_get_phys(unsigned int virt);

/*
 * temp_map(phys)
 *
 * Mapeia temporariamente o frame físico 'phys' no endereço virtual fixo
 * TEMP_MAP_VADDR (0xC07FF000). Retorna o ponteiro virtual para esse frame.
 *
 * Uso típico (Capítulo 10.2):
 *   unsigned int frame = pfa_alloc();
 *   void *ptr = temp_map(frame);
 *   memset(ptr, 0, PAGE_SIZE);   // inicializa o frame
 *   temp_unmap();
 *
 * AVISO: só existe UM slot temporário. Não chame temp_map() duas vezes
 * sem temp_unmap() no meio — o mapeamento anterior é sobrescrito.
 */
void *temp_map(unsigned int phys);

/*
 * temp_unmap()
 *
 * Remove o mapeamento temporário criado por temp_map() e invalida o TLB.
 */
void temp_unmap(void);

#endif /* PAGING_H */