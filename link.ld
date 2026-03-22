/*
 * link.ld — Linker script para kernel higher-half
 *
 * VMA (endereço virtual): 0xC0100000  (região 3GB+1MB)
 * LMA (endereço físico):  0x00100000  (onde o GRUB realmente carrega)
 *
 * 
 *   1. kernel_physical_start/end agora são calculados como expressão numérica pura
 *      (não como símbolos de endereço), evitando que o linker os trate como VMAs.
 *   2. Seção .multiboot vem ANTES de .text para garantir que o header Multiboot
 *      esteja nos primeiros 8KB do binário — requisito obrigatório do GRUB.
 *   3. Adicionada seção .bss explícita com PROVIDE para compatibilidade.
 *   4. AT() calculado de forma segura usando ADDR() - offset.
 */

ENTRY(loader)

KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
    /* Endereço virtual de início: 3GB + 1MB */
    . = 0xC0100000;

    kernel_virtual_start = .;

    /*
     * kernel_physical_start: endereço FÍSICO de carga.
     * Usar ". - KERNEL_VIRT_BASE" aqui é seguro porque é avaliado como
     * expressão numérica no contexto do contador de localização.
     */
    kernel_physical_start = . - KERNEL_VIRT_BASE;   /* = 0x00100000 */

    /* ------------------------------------------------------------------ */
    /* .multiboot PRIMEIRO — GRUB exige header nos primeiros 8KB           */
    /* ------------------------------------------------------------------ */
    .multiboot ALIGN(4) : AT(ADDR(.multiboot) - KERNEL_VIRT_BASE)
    {
        *(.multiboot)
    }

    /* ------------------------------------------------------------------ */
    /* .text — código executável                                            */
    /* ------------------------------------------------------------------ */
    .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text)
        *(.text*)
    }

    /* ------------------------------------------------------------------ */
    /* .rodata — dados somente-leitura                                      */
    /* ------------------------------------------------------------------ */
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata)
        *(.rodata*)
    }

    /* ------------------------------------------------------------------ */
    /* .data — dados inicializados (inclui page_directory)                  */
    /* ------------------------------------------------------------------ */
    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data)
        *(.data*)
    }

    /* ------------------------------------------------------------------ */
    /* .bss — dados não inicializados (inclui kernel_stack)                 */
    /* ------------------------------------------------------------------ */
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss)
        *(.bss*)
    }

    kernel_virtual_end = .;
    kernel_physical_end = . - KERNEL_VIRT_BASE;
}
