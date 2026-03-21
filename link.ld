ENTRY(loader)

SECTIONS
{
    /* O kernel começa virtualmente em 3GB + 1MB */
    . = 0xC0100000;

    kernel_virtual_start = .;
    /* O endereço físico real subtrai o offset de 3GB */
    kernel_physical_start = . - 0xC0000000;

    /* AT() diz ao linker: "O endereço virtual é X, mas grave o binário físico em Y" */
    .multiboot ALIGN (0x4) : AT(ADDR(.multiboot) - 0xC0000000) {
        *(.multiboot)
    }

    .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
        *(.text*)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata)
        *(.rodata*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)
        *(.data*)
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xC0000000) {
        *(COMMON)
        *(.bss)
        *(.bss*)
    }

    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;
}