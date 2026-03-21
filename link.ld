ENTRY(loader)

SECTIONS
{
    . = 0x00100000;

    kernel_virtual_start = .;
    kernel_physical_start = .;

    .multiboot ALIGN (0x4) : {
        *(.multiboot)
    }

    .text ALIGN (0x1000) : {
        *(.text)
        *(.text*)
    }

    .rodata ALIGN (0x1000) : {
        *(.rodata)
        *(.rodata*)
    }

    .data ALIGN (0x1000) : {
        *(.data)
        *(.data*)
    }

    .bss ALIGN (0x1000) : {
        *(COMMON)
        *(.bss)
        *(.bss*)
    }

    kernel_virtual_end = .;
    kernel_physical_end = .;
}