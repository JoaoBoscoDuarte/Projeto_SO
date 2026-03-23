#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "multiboot.h"
#include "pfa.h"
#include "paging.h"

typedef void (*call_module_t)(void);

/* ============================================================================
 * paging_check() — Diagnóstico do subsistema de paginação
 *
 * Verifica se paging_init() configurou corretamente as page tables.
 * Todo output vai para a serial para não depender do framebuffer.
 *
 * Resultados esperados:
 *   PD[0]   = 0x0         → identity map removido
 *   PD[768] PSE bit = 0   → page table de 4KB (não PSE 4MB)
 *   PD[768] Present = 1   → entrada presente
 *   PD[769] != 0          → temp_pt instalada
 *   0xC0100000 → 0x100000 → kernel mapeado corretamente
 *   0xC00B8000 → 0xB8000  → VGA acessível
 *   0xD0000000 → 0x0      → endereço não mapeado retorna 0
 * ============================================================================ */
static void paging_check(void)
{
    extern unsigned int page_directory[1024];

    kprintf(OUTPUT_SERIAL, "\n=== DIAGNÓSTICO DE PAGINAÇÃO ===\n");

    /* 1. Entrada 0 deve estar zerada (identity map removido) */
    kprintf(OUTPUT_SERIAL, "PD[0]          = 0x%x  (esperado: 0x0)\n",
            page_directory[0]);

    /* 2. Entrada 768 — kernel, page table de 4KB */
    unsigned int pde768 = page_directory[768];
    kprintf(OUTPUT_SERIAL, "PD[768]        = 0x%x\n", pde768);
    kprintf(OUTPUT_SERIAL, "  PSE bit      = %d     (esperado: 0 = 4KB PT)\n",
            (pde768 >> 7) & 1);
    kprintf(OUTPUT_SERIAL, "  Present bit  = %d     (esperado: 1)\n",
            pde768 & 1);

    /* 3. Entrada 769 — temp_pt */
    unsigned int pde769 = page_directory[769];
    kprintf(OUTPUT_SERIAL, "PD[769]        = 0x%x  (esperado: != 0)\n",
            pde769);

    /* 4. Traducoes virtuais -> fisicas */
    kprintf(OUTPUT_SERIAL, "\n--- traducoes de endereco ---\n");

    unsigned int k_phys = paging_get_phys(0xC0100000);
    kprintf(OUTPUT_SERIAL, "0xC0100000  -> 0x%x  (esperado: 0x100000)\n",
            k_phys);

    unsigned int vga_phys = paging_get_phys(0xC00B8000);
    kprintf(OUTPUT_SERIAL, "0xC00B8000  -> 0x%x  (esperado: 0xB8000)\n",
            vga_phys);

    unsigned int zero_phys = paging_get_phys(0xC0000000);
    kprintf(OUTPUT_SERIAL, "0xC0000000  -> 0x%x  (esperado: 0x0)\n",
            zero_phys);

    unsigned int unmapped = paging_get_phys(0xD0000000);
    kprintf(OUTPUT_SERIAL, "0xD0000000  -> 0x%x  (esperado: 0x0 = nao mapeado)\n",
            unmapped);

    /* 5. Resumo */
    kprintf(OUTPUT_SERIAL, "\n--- resultado ---\n");
    unsigned int ok =
        (page_directory[0] == 0) &&
        ((pde768 & 1) == 1) &&
        (((pde768 >> 7) & 1) == 0) &&
        (pde769 != 0) &&
        (k_phys == 0x100000) &&
        (vga_phys == 0xB8000) &&
        (unmapped == 0);

    if (ok)
        kprintf(OUTPUT_SERIAL, "[OK] Paginacao configurada corretamente\n");
    else
        kprintf(OUTPUT_SERIAL, "[ERRO] Falha na verificacao da paginacao\n");

    kprintf(OUTPUT_SERIAL, "==========================\n\n");
}

/* ============================================================================
 * kmain() — Ponto de entrada do kernel em C
 *
 * Recebe argumentos empurrados pelo loader.s (em ordem cdecl):
 *   multiboot_addr       : endereço FÍSICO da struct multiboot_info
 *   kernel_virtual_start : VMA do início do kernel  (ex.: 0xC0100000)
 *   kernel_virtual_end   : VMA do fim do kernel
 *   kernel_physical_start: LMA do início do kernel  (ex.: 0x00100000)
 *   kernel_physical_end  : LMA do fim do kernel
 *
 * REGRA DE OURO — endereços no higher-half:
 *   Ponteiros físicos recebidos do GRUB precisam de +0xC0000000 antes
 *   de serem derreferenciados. A entrada 768 do PD mapeia os primeiros
 *   4MB: físico 0x0–0x3FFFFF ↔ virtual 0xC0000000–0xC03FFFFF.
 * ============================================================================ */
void kmain(unsigned int multiboot_addr,
           unsigned int kernel_virtual_start,
           unsigned int kernel_virtual_end,
           unsigned int kernel_physical_start,
           unsigned int kernel_physical_end)
{
    /* ------------------------------------------------------------------
     * 1. Converte multiboot_addr físico → virtual ANTES de qualquer
     *    derreferenciamento. Sem isso: page fault imediato.
     * ------------------------------------------------------------------ */
    multiboot_info_t *mbinfo =
        (multiboot_info_t *)(multiboot_addr + 0xC0000000u);

    unsigned int flags             = mbinfo->flags;
    unsigned int number_of_modules = mbinfo->mods_count;

    /* mods_addr dentro da struct também é físico — converte agora */
    multiboot_module_t *mods = (multiboot_module_t *) 0;
    if ((flags & MULTIBOOT_INFO_MODS) && number_of_modules > 0) {
        mods = (multiboot_module_t *)(mbinfo->mods_addr + 0xC0000000u);
    }

    /* ------------------------------------------------------------------
     * 2. Inicialização dos subsistemas
     *
     *    ORDEM OBRIGATÓRIA:
     *      a) fb_clear   — limpa tela (não depende de nada)
     *      b) pfa_init   — inicializa bitmap de frames físicos
     *      c) paging_init— troca PSE 4MB por page tables reais de 4KB
     *                      (pode chamar pfa_alloc internamente)
     *      d) demais drivers
     * ------------------------------------------------------------------ */
    fb_clear();

    paging_init();   // primeiro (ativa higher-half de verdade)
    pfa_init(kernel_physical_start, kernel_physical_end, mbinfo);

    gdt_init();
    serial_init();
    idt_init();
    pic_remap();

    /* ------------------------------------------------------------------
     * 3. Diagnóstico da paginação (output na serial)
     *    Remove ou comenta este bloco quando não for mais necessário.
     * ------------------------------------------------------------------ */
    paging_check();

    /* ------------------------------------------------------------------
     * 4. Mensagens de inicialização
     * ------------------------------------------------------------------ */
    log_info("Sistema Operacional Iniciado");

    kprintf(OUTPUT_FB, "kernel virtual:  0x%x - 0x%x\n",
            kernel_virtual_start, kernel_virtual_end);
    kprintf(OUTPUT_FB, "kernel physical: 0x%x - 0x%x\n",
            kernel_physical_start, kernel_physical_end);
    kprintf(OUTPUT_FB, "Tamanho do kernel: %d KB\n",
            (kernel_virtual_end - kernel_virtual_start) / 1024);

    /* ------------------------------------------------------------------
     * 5. Testes do PFA (Page Frame Allocator)
     * ------------------------------------------------------------------ */
    unsigned int f1 = pfa_alloc();
    unsigned int f2 = pfa_alloc();
    unsigned int f3 = pfa_alloc();
    kprintf(OUTPUT_FB, "frame1: 0x%x\n", f1);
    kprintf(OUTPUT_FB, "frame2: 0x%x\n", f2);
    kprintf(OUTPUT_FB, "frame3: 0x%x\n", f3);

    pfa_free(f1);
    unsigned int f4 = pfa_alloc();   /* deve reutilizar f1 */
    kprintf(OUTPUT_FB, "frame4 (reuso f1): 0x%x\n", f4);

    kprintf(OUTPUT_FB, "PFA total_frames: %u\n", pfa_total_frames());
    kprintf(OUTPUT_FB, "PFA bitmap phys: 0x%x - 0x%x\n",
        pfa_bitmap_start(), pfa_bitmap_end());

    /* ------------------------------------------------------------------
     * 6. Teste de mapeamento temporário — Capítulo 10.2
     *
     *    pfa_alloc() retorna endereço FÍSICO.
     *    temp_map() mapeia esse frame em 0xC07FF000 temporariamente,
     *    permitindo escrita/leitura antes de mapeá-lo definitivamente.
     * ------------------------------------------------------------------ */
    unsigned int test_frame = pfa_alloc();
    kprintf(OUTPUT_FB, "temp_map frame: 0x%x\n", test_frame);
    if (test_frame != 0) {
        unsigned int *ptr = (unsigned int *) temp_map(test_frame);
        ptr[0] = 0xDEADBEEF;
        ptr[1] = 0xC0FFEE00;
        kprintf(OUTPUT_FB, "temp_map[0]: 0x%x\n", ptr[0]);
        kprintf(OUTPUT_FB, "temp_map[1]: 0x%x\n", ptr[1]);
        temp_unmap();
        pfa_free(test_frame);
    }

    /* ------------------------------------------------------------------
     * 7. Informações do multiboot
     * ------------------------------------------------------------------ */
    kprintf(OUTPUT_FB, "\nFlags: %d\n", flags);
    kprintf(OUTPUT_FB, "Modulos: %d\n", number_of_modules);

    /* ------------------------------------------------------------------
     * 8. Executa módulo GRUB (se existir)
     *
     *    mods[0].mod_start é FÍSICO. Converte para virtual somando
     *    0xC0000000 — funciona enquanto o módulo estiver nos primeiros
     *    4MB (cobertos pela entrada 768 do PD).
     * ------------------------------------------------------------------ */
    if (mods != (multiboot_module_t *) 0) {
        kprintf(OUTPUT_FB, "Modulo em phys: 0x%x\n", mods[0].mod_start);
        unsigned int module_virt = mods[0].mod_start + 0xC0000000u;
        call_module_t start_program = (call_module_t) module_virt;
        start_program();
    }

    /* ------------------------------------------------------------------
     * 9. Loop final — habilita interrupções e aguarda
     * ------------------------------------------------------------------ */
    log_debug("Drivers carregados com sucesso");
    log_error("Teste de mensagem de erro");

    asm volatile("sti");
    while (1) {
        asm volatile("hlt");
    }
}