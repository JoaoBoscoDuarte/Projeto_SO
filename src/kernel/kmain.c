#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "tss.h"
#include "kheap.h"
#include "pic.h"
#include "idt.h"
#include "multiboot.h"
#include "pfa.h"
#include "paging.h"
#include "process.h"

/* kernel_stack definido em loader.s (.bss) — topo = kernel_stack + 4096 */
extern unsigned char kernel_stack[];

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

    /* 4. Traduções virtuais -> físicas */
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
 *   kernel_virtual_start : VMA do início do kernel
 *   kernel_virtual_end   : VMA do fim do kernel
 *   kernel_physical_start: LMA do início do kernel
 *   kernel_physical_end  : LMA do fim do kernel
 *
 * REGRA DE OURO — higher-half kernel:
 *   Estruturas apontadas pelo GRUB chegam com endereços físicos e devem ser
 *   convertidas para virtual antes de serem dereferenciadas.
 * ============================================================================ */
void kmain(unsigned int multiboot_addr,
           unsigned int kernel_virtual_start,
           unsigned int kernel_virtual_end,
           unsigned int kernel_physical_start,
           unsigned int kernel_physical_end)
{
    /* ------------------------------------------------------------------
     * 1. Conversão inicial de estruturas Multiboot
     * ------------------------------------------------------------------ */
    multiboot_info_t *mbinfo =
        (multiboot_info_t *)(multiboot_addr + 0xC0000000u);

    unsigned int flags             = mbinfo->flags;
    unsigned int number_of_modules = mbinfo->mods_count;

    multiboot_module_t *mods = (multiboot_module_t *)0;
    if ((flags & MULTIBOOT_INFO_MODS) && number_of_modules > 0) {
        mods = (multiboot_module_t *)(mbinfo->mods_addr + 0xC0000000u);
    }

    /* ------------------------------------------------------------------
     * 2. Inicialização base do kernel
     *
     * Fluxo atual:
     *   a) fb_clear    → prepara a saída visual
     *   b) paging_init → instala page tables reais de 4KB
     *   c) pfa_init    → inicializa o allocator de frames físicos
     *   d) kheap_init  → cria a heap do kernel sobre paging + PFA
     * ------------------------------------------------------------------ */
    fb_clear();

    paging_init();
    pfa_init(kernel_physical_start, kernel_physical_end, mbinfo);
    kheap_init();

    /* ------------------------------------------------------------------
     * 3. Inicialização de descritores, interrupções e I/O básico
     * ------------------------------------------------------------------ */
    gdt_init();

    /* TSS: kernel_stack é o início do buffer de 4096 bytes;
     * o topo (ESP inicial) está em kernel_stack + 4096. */
    tss_init(GDT_KERNEL_DATA, (unsigned int)(kernel_stack + 4096));

    serial_init();
    idt_init();
    pic_remap();

    /* ------------------------------------------------------------------
     * 4. Diagnóstico da paginação
     * ------------------------------------------------------------------ */
    paging_check();

    /* ------------------------------------------------------------------
     * 5. Informações gerais de boot
     * ------------------------------------------------------------------ */
    log_info("Sistema Operacional Iniciado");

    kprintf(OUTPUT_FB, "kernel virtual:  0x%x - 0x%x\n",
            kernel_virtual_start, kernel_virtual_end);
    kprintf(OUTPUT_FB, "kernel physical: 0x%x - 0x%x\n",
            kernel_physical_start, kernel_physical_end);
    kprintf(OUTPUT_FB, "Tamanho do kernel: %d KB\n",
            (kernel_virtual_end - kernel_virtual_start) / 1024);

    /* ------------------------------------------------------------------
     * 6. Testes do Page Frame Allocator (PFA)
     *
     * Valida:
     *   - alocação sequencial de frames físicos
     *   - reutilização após free
     *   - exposição de metadados do allocator
     * ------------------------------------------------------------------ */
    unsigned int f1 = pfa_alloc();
    unsigned int f2 = pfa_alloc();
    unsigned int f3 = pfa_alloc();

    kprintf(OUTPUT_FB, "frame1: 0x%x\n", f1);
    kprintf(OUTPUT_FB, "frame2: 0x%x\n", f2);
    kprintf(OUTPUT_FB, "frame3: 0x%x\n", f3);

    pfa_free(f1);
    unsigned int f4 = pfa_alloc();

    kprintf(OUTPUT_FB, "frame4 (reuso f1): 0x%x\n", f4);
    kprintf(OUTPUT_FB, "PFA total_frames: %d\n", pfa_total_frames());
    kprintf(OUTPUT_FB, "PFA bitmap phys: 0x%x - 0x%x\n",
            pfa_bitmap_start(), pfa_bitmap_end());

    /* ------------------------------------------------------------------
     * 7. Testes da kernel heap
     *
     * Valida:
     *   - inicialização da heap
     *   - alocação dinâmica em bytes
     *   - retorno de ponteiros na região virtual do heap
     * ------------------------------------------------------------------ */
    void *a = kmalloc(32);
    void *b = kmalloc(64);

    kprintf(OUTPUT_FB, "a: 0x%x\n", a);
    kprintf(OUTPUT_FB, "b: 0x%x\n", b);

    kfree(a);

    /* ------------------------------------------------------------------
     * 8. Teste de mapeamento temporário — temp_map()
     *
     * pfa_alloc() retorna endereço físico.
     * temp_map() expõe esse frame em uma janela virtual fixa para acesso
     * temporário em software.
     * ------------------------------------------------------------------ */
    unsigned int test_frame = pfa_alloc();
    kprintf(OUTPUT_FB, "temp_map frame: 0x%x\n", test_frame);

    if (test_frame != 0) {
        unsigned int *ptr = (unsigned int *)temp_map(test_frame);

        ptr[0] = 0xDEADBEEF;
        ptr[1] = 0xC0FFEE00;

        kprintf(OUTPUT_FB, "temp_map[0]: 0x%x\n", ptr[0]);
        kprintf(OUTPUT_FB, "temp_map[1]: 0x%x\n", ptr[1]);

        temp_unmap();
        pfa_free(test_frame);
    }

    /* ------------------------------------------------------------------
     * 9. Informações de Multiboot e módulos carregados
     * ------------------------------------------------------------------ */
    kprintf(OUTPUT_FB, "\nFlags: %d\n", flags);
    kprintf(OUTPUT_FB, "Modulos: %d\n", number_of_modules);

    /* ------------------------------------------------------------------
     * 10. Inicia processo user mode a partir do módulo GRUB
     *
     *    mods[0].mod_start é endereço FÍSICO do binário flat carregado
     *    pelo GRUB. process_create() aloca um page directory próprio,
     *    copia o código e configura a stack do processo.
     *    enter_usermode() faz a transição ring 0 → ring 3 via iret.
     * ------------------------------------------------------------------ */
    if (mods != (multiboot_module_t *) 0) {
        unsigned int mod_start = mods[0].mod_start;
        unsigned int mod_end   = mods[0].mod_end;
        unsigned int mod_size  = mod_end - mod_start;

        kprintf(OUTPUT_FB, "Modulo: phys 0x%x - 0x%x (%d bytes)\n",
                mod_start, mod_end, mod_size);

        struct process proc = process_create(mod_start, mod_size);

        if (proc.page_directory_phys == 0) {
            log_error("Falha ao criar processo user mode");
        } else {
            kprintf(OUTPUT_FB, "Processo criado: eip=0x%x esp=0x%x pd=0x%x\n",
                    proc.eip, proc.esp, proc.page_directory_phys);
            log_info("Entrando em user mode...");

            /* Habilita interrupções antes de entrar em user mode:
             * iret carregará eflags com IF=1 (definido em usermode.s),
             * mas o PIC já está configurado — sem sti aqui o teclado
             * funciona somente após o iret. */
            asm volatile("sti");

            enter_usermode(proc.eip, proc.esp, proc.page_directory_phys);
            /* enter_usermode() não retorna */
        }
    }

    /* ------------------------------------------------------------------
     * 11. Loop final — habilita interrupções e aguarda
     * ------------------------------------------------------------------ */
    log_debug("Drivers carregados com sucesso");

    asm volatile("sti");
    while (1) {
        asm volatile("hlt");
    }
}