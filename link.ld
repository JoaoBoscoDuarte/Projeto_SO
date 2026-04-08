/*
 * link.ld — Linker script para kernel higher-half x86 32-bit
 *
 * CONCEITO CENTRAL: VMA vs LMA
 *   VMA (Virtual Memory Address): endereço que o código usa em tempo de execução.
 *   LMA (Load Memory Address):    endereço onde o GRUB realmente grava o binário.
 *
 *   VMA do kernel: 0xC0100000  (3 GB + 1 MB — "higher-half")
 *   LMA do kernel: 0x00100000  (1 MB físico — onde o GRUB carrega)
 *
 *   O kernel é carregado em 0x00100000, mas todos os símbolos (funções,
 *   variáveis globais) têm endereços a partir de 0xC0100000. Isso é possível
 *   porque o loader.s ativa paginação antes de chamar qualquer código C,
 *   mapeando 0xC0000000 → 0x00000000 (entrada 768 do page directory).
 *
 *   A diretiva AT() em cada seção instrui o linker a usar a LMA correta:
 *     AT(ADDR(.section) - KERNEL_VIRT_BASE)
 *   Isso garante que o GRUB grave os bytes no endereço físico certo.
 *
 * ORDEM DAS SEÇÕES:
 *   .multiboot deve vir PRIMEIRO — o GRUB procura o header Multiboot
 *   nos primeiros 8 KB do binário. Se .text vier antes, o header pode
 *   ficar além desse limite e o GRUB não reconhecerá o kernel.
 *
 * SÍMBOLOS EXPORTADOS:
 *   kernel_virtual_start/end  — usados pelo kmain para exibir o layout
 *   kernel_physical_start/end — passados ao pfa_init() para marcar o
 *                               kernel como região ocupada no bitmap
 */

ENTRY(loader)

KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
    /* Endereço virtual de início: 3 GB + 1 MB */
    . = 0xC0100000;

    kernel_virtual_start  = .;
    kernel_physical_start = . - KERNEL_VIRT_BASE;   /* = 0x00100000 */

    /* ------------------------------------------------------------------
     * .multiboot — cabeçalho Multiboot
     *
     * Deve estar nos primeiros 8 KB do binário (requisito do GRUB).
     * Contém: magic number (0x1BADB002), flags e checksum.
     * O GRUB verifica magic + flags + checksum == 0 antes de carregar.
     * ------------------------------------------------------------------ */
    .multiboot ALIGN(4) : AT(ADDR(.multiboot) - KERNEL_VIRT_BASE)
    {
        *(.multiboot)
    }

    /* ------------------------------------------------------------------
     * .text — código executável
     *
     * Alinhado em 4 KB (tamanho de uma página) para que paging_init()
     * possa mapear o kernel com granularidade de página.
     * ------------------------------------------------------------------ */
    .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text)
        *(.text*)
    }

    /* ------------------------------------------------------------------
     * .rodata — dados somente-leitura
     *
     * Strings literais, tabelas constantes (ex: kbd_map[], accents[]).
     * Separado de .text para permitir proteção de escrita no futuro.
     * ------------------------------------------------------------------ */
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata)
        *(.rodata*)
    }

    /* ------------------------------------------------------------------
     * .data — dados inicializados
     *
     * Variáveis globais com valor inicial definido em código.
     * Inclui page_directory[] (definido em loader.s, seção .data),
     * que precisa estar alinhado em 4096 bytes para ser carregado em CR3.
     * ------------------------------------------------------------------ */
    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data)
        *(.data*)
    }

    /* ------------------------------------------------------------------
     * .bss — dados não inicializados
     *
     * Variáveis globais sem valor inicial — zeradas automaticamente
     * pelo loader antes de chamar kmain.
     * Inclui kernel_stack[] (4 KB, definido em loader.s, seção .bss).
     * COMMON: símbolos "comuns" do C (variáveis globais sem inicializador
     * em múltiplos arquivos objeto).
     * ------------------------------------------------------------------ */
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss)
        *(.bss*)
    }

    kernel_virtual_end  = .;
    kernel_physical_end = . - KERNEL_VIRT_BASE;
}
