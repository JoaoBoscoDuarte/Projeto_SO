#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "tss.h"
#include "pic.h"
#include "idt.h"
#include "pit.h"
#include "multiboot.h"
#include "pfa.h"
#include "paging.h"
#include "kheap.h"
#include "process.h"
#include "shell.h"

void kmain(unsigned int multiboot_addr,
           unsigned int kernel_virtual_start __attribute__((unused)),
           unsigned int kernel_virtual_end,
           unsigned int kernel_physical_start,
           unsigned int kernel_physical_end)
{
    multiboot_info_t *mbinfo =
        (multiboot_info_t *)(multiboot_addr + 0xC0000000u);

    fb_clear();

    pfa_init(kernel_physical_start, kernel_physical_end, mbinfo);
    paging_init();
    gdt_init();
    tss_init(0x10, kernel_virtual_end);
    serial_init();
    idt_init();
    pic_remap();
    pit_init(100);
    kheap_init();
    process_init();

    log_info("Sistema pronto. Iniciando shell...");

    asm volatile("sti");
    shell_run();

    /* shell_run nunca retorna; fallback */
    while (1) asm volatile("hlt");
}
