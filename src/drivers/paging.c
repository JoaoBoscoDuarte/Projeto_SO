/* ============================================================================
 * paging.c — Implementação do subsistema de paginação x86
 *
 * Estratégia de implementação:
 *
 *   O loader.s já ativou paginação usando PSE (4MB pages):
 *     - Entrada 0   do PD: identity map temporário (0x0 → 0x0, 4MB)    ← JÁ REMOVIDA
 *     - Entrada 768 do PD: higher-half             (0xC0000000 → 0x0)   ← PSE ativa
 *
 *   paging_init() substitui esse setup por page tables reais de 4KB:
 *     - Entrada 768: page table real cobrindo 0xC0000000–0xC03FFFFF
 *     - Entrada 769: page table para mapeamentos temporários
 *
 *   Isso é necessário porque:
 *     a) PSE mapeia 4MB inteiros — não dá para mapear/desmapar páginas individuais
 *     b) Precisamos do slot de mapeamento temporário (Cap. 10.2)
 *     c) Page tables de 4KB são o fundamento para o heap do kernel e user mode
 *
 * Dependências:
 *   - pfa_alloc() / pfa_free()   → alocação de frames físicos
 *   - loader.s                   → page_directory já carregado em CR3
 * ========================================================================= */

#include "paging.h"
#include "pfa.h"

/* --------------------------------------------------------------------------
 * Acesso ao page_directory definido em loader.s
 *
 * O símbolo 'page_directory' tem endereço VIRTUAL (0xC01xxxxx) porque está
 * na seção .data do kernel, que foi linkada com VMA = 0xC0100000+.
 * Após paging_init() ser chamada em kmain(), o PD usado pelo hardware é este.
 * -------------------------------------------------------------------------- */
extern pde_t page_directory[1024];

/* --------------------------------------------------------------------------
 * Page tables estáticas para as duas regiões que precisamos de imediato:
 *
 *   kernel_pt  : cobre 0xC0000000–0xC03FFFFF (entrada 768 do PD)
 *   temp_pt    : cobre 0xC0400000–0xC07FFFFF (entrada 769 do PD)
 *
 * São alocadas estaticamente (em .bss) para evitar dependência do pfa durante
 * a inicialização, quando o pfa já está pronto mas o temp_map ainda não existe.
 *
 * ALINHAMENTO EM 4096 BYTES É OBRIGATÓRIO — o hardware ignora os 12 bits
 * baixos de um PDE/PTE; eles são usados para flags.
 * -------------------------------------------------------------------------- */
static pte_t kernel_pt[1024]   __attribute__((aligned(4096)));
static pte_t temp_pt[1024]     __attribute__((aligned(4096)));

/* --------------------------------------------------------------------------
 * Helpers internos
 * -------------------------------------------------------------------------- */

/* Invalida uma única entrada do TLB para o endereço virtual 'vaddr'. */
static inline void tlb_flush_single(unsigned int vaddr)
{
    asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

/* Recarrega CR3 (flush COMPLETO do TLB — invalida todas as entradas). */
static inline void tlb_flush_all(void)
{
    unsigned int cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

/* Lê o endereço físico atual em CR3. */
static inline unsigned int cr3_read(void)
{
    unsigned int val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

/* Escreve um novo endereço físico em CR3 (troca o page directory). */
static inline void cr3_write(unsigned int phys_addr)
{
    asm volatile("mov %0, %%cr3" :: "r"(phys_addr) : "memory");
}

/* --------------------------------------------------------------------------
 * Obtém um ponteiro virtual para a page table apontada pela PDE 'pde'.
 *
 * A PDE armazena o endereço FÍSICO da page table.
 * Para acessar a page table em C, precisamos do endereço VIRTUAL.
 * Como nossa page table do kernel foi alocada estaticamente em .bss,
 * o endereço físico = VMA - KERNEL_VIRT_BASE.
 * Portanto: VMA = físico + KERNEL_VIRT_BASE.
 *
 * Para page tables alocadas dinamicamente via pfa_alloc (que retorna físico),
 * a mesma conversão se aplica — mas só funciona enquanto a região física
 * estiver dentro dos primeiros 4MB (cobertos pela entrada 768).
 * Para frames fora dessa janela, usa-se temp_map antes de acessar.
 * -------------------------------------------------------------------------- */
static inline pte_t *pde_to_pt_virt(pde_t pde)
{
    unsigned int phys = PDE_FRAME(pde);
    return (pte_t *)((unsigned int)phys + KERNEL_VIRT_BASE);
}

/* ============================================================================
 * paging_init()
 * ========================================================================= */
void paging_init(void)
{
    unsigned int i;
    unsigned int phys;

    /* ------------------------------------------------------------------
     * Passo 1: Zera as duas page tables estáticas.
     * ------------------------------------------------------------------ */
    for (i = 0; i < 1024; i++) {
        kernel_pt[i] = 0;
        temp_pt[i]   = 0;
    }

    /* ------------------------------------------------------------------
     * Passo 2: Popula kernel_pt — mapeia os primeiros 4MB de RAM para
     * o endereço virtual 0xC0000000–0xC03FFFFF.
     *
     * Entrada i da page table cobre:
     *   virtual:  0xC0000000 + i * 4096
     *   físico:   0x00000000 + i * 4096
     *
     * Mapeamos todas as 1024 páginas (4MB) com flags PAGE_KERNEL.
     * Isso garante que todo o kernel (que está fisicamente em 0x00100000–...)
     * e os primeiros 1MB (BIOS/VGA em 0xB8000, etc.) sejam acessíveis.
     * ------------------------------------------------------------------ */
    for (i = 0; i < 1024; i++) {
        phys = i * PAGE_SIZE;
        kernel_pt[i] = phys | PAGE_KERNEL;
    }

    /* ------------------------------------------------------------------
     * Passo 3: Instala kernel_pt na entrada 768 do page directory.
     *
     * ATENÇÃO: a PDE deve conter o endereço FÍSICO da page table.
     * kernel_pt é um símbolo em .bss → endereço virtual 0xC01xxxxx.
     * Físico = virtual - KERNEL_VIRT_BASE.
     *
     * NÃO usamos PAGE_PSE aqui — esta é uma page table de 4KB, não 4MB.
     * ------------------------------------------------------------------ */
    phys = VIRT_TO_PHYS(kernel_pt);
    page_directory[KERNEL_PD_INDEX] = phys | PAGE_KERNEL;

    /* ------------------------------------------------------------------
     * Passo 4: Instala temp_pt na entrada 769 do page directory.
     * A página de mapeamento temporário será a entrada 1023 desta PT.
     * Deixamos a PT zerada (sem mapeamentos) por enquanto.
     * ------------------------------------------------------------------ */
    phys = VIRT_TO_PHYS(temp_pt);
    page_directory[TEMP_MAP_PD_INDEX] = phys | PAGE_KERNEL;

    /* ------------------------------------------------------------------
     * Passo 5: Garante que o identity map (entrada 0) está removido.
     * O loader.s já deveria ter feito isso, mas fazemos aqui por segurança.
     * ------------------------------------------------------------------ */
    page_directory[0] = 0;

    /* ------------------------------------------------------------------
     * Passo 6: Flush completo do TLB.
     * As mudanças no PD (entrada 768: PSE→PT real, entrada 769: nova PT)
     * não terão efeito até o TLB ser invalidado.
     * ------------------------------------------------------------------ */
    tlb_flush_all();
}

/* ============================================================================
 * paging_map(virt, phys, flags)
 * ========================================================================= */
int paging_map(unsigned int virt, unsigned int phys, unsigned int flags)
{
    unsigned int pd_idx = PD_INDEX(virt);
    unsigned int pt_idx = PT_INDEX(virt);
    pte_t *pt;

    /* Garante que os endereços estão alinhados */
    virt  = PAGE_ALIGN_DOWN(virt);
    phys  = PAGE_ALIGN_DOWN(phys);

    if (page_directory[pd_idx] & PAGE_PRESENT) {
        /* Page table já existe — obtém ponteiro virtual para ela */
        pt = pde_to_pt_virt(page_directory[pd_idx]);
    } else {
        /*
         * Page table não existe — precisa alocar um novo frame e inicializá-lo.
         *
         * PROBLEMA: pfa_alloc() retorna um endereço físico. Para escrever zeros
         * nesse frame, precisamos de um mapeamento virtual → usamos temp_map().
         *
         * FLUXO:
         *   1. aloca frame físico para a nova page table
         *   2. mapeia-o temporariamente via temp_map
         *   3. zera os 4096 bytes
         *   4. remove o mapeamento temporário
         *   5. instala o frame no page directory
         */
        unsigned int new_pt_phys = pfa_alloc();
        if (new_pt_phys == 0) {
            return -1;  /* sem frames disponíveis */
        }

        /* Zera a nova page table usando o mapeamento temporário */
        pte_t *new_pt = (pte_t *)temp_map(new_pt_phys);
        unsigned int j;
        for (j = 0; j < 1024; j++) {
            new_pt[j] = 0;
        }
        temp_unmap();

        /* Instala a nova page table no page directory */
        page_directory[pd_idx] = new_pt_phys | PAGE_KERNEL;
        tlb_flush_all();  /* invalida PD para que o hardware veja a nova PT */

        /* Agora podemos obter o ponteiro virtual para ela */
        pt = pde_to_pt_virt(page_directory[pd_idx]);
    }

    /* Escreve a entrada na page table */
    pt[pt_idx] = (phys & 0xFFFFF000u) | (flags | PAGE_PRESENT);

    /* Invalida somente a página afetada no TLB */
    tlb_flush_single(virt);

    return 0;
}

/* ============================================================================
 * paging_unmap(virt)
 * ========================================================================= */
void paging_unmap(unsigned int virt)
{
    unsigned int pd_idx = PD_INDEX(virt);
    unsigned int pt_idx = PT_INDEX(virt);

    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        return;  /* page table não existe — nada a fazer */
    }

    pte_t *pt = pde_to_pt_virt(page_directory[pd_idx]);
    pt[pt_idx] = 0;

    tlb_flush_single(virt);
}

/* ============================================================================
 * paging_get_phys(virt)
 * ========================================================================= */
unsigned int paging_get_phys(unsigned int virt)
{
    unsigned int pd_idx = PD_INDEX(virt);
    unsigned int pt_idx = PT_INDEX(virt);

    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        return 0;
    }

    pte_t *pt = pde_to_pt_virt(page_directory[pd_idx]);

    if (!(pt[pt_idx] & PAGE_PRESENT)) {
        return 0;
    }

    return PTE_FRAME(pt[pt_idx]);
}

/* ============================================================================
 * temp_map(phys) — Capítulo 10.2
 *
 * Mapeia o frame físico 'phys' no slot fixo TEMP_MAP_VADDR (0xC07FF000).
 * Usa a entrada 1023 da temp_pt (página table da entrada 769 do PD).
 *
 * Por que funciona:
 *   - temp_pt está instalada na entrada 769 do PD
 *   - TEMP_MAP_VADDR = 0xC07FF000 → PD index = 769, PT index = 1023
 *   - Escrevemos o frame físico na temp_pt[1023] com flags PAGE_KERNEL
 *   - INVLPG invalida a entrada do TLB para 0xC07FF000
 *   - O hardware traduz 0xC07FF000 → 'phys' nos próximos acessos
 * ========================================================================= */
void *temp_map(unsigned int phys)
{
    temp_pt[TEMP_MAP_PT_INDEX] = (phys & 0xFFFFF000u) | PAGE_KERNEL;
    tlb_flush_single(TEMP_MAP_VADDR);
    return (void *)TEMP_MAP_VADDR;
}

/* ============================================================================
 * temp_unmap()
 * ========================================================================= */
void temp_unmap(void)
{
    temp_pt[TEMP_MAP_PT_INDEX] = 0;
    tlb_flush_single(TEMP_MAP_VADDR);
}