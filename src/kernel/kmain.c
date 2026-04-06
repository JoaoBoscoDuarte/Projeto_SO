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
#include "scheduler.h"

/* Funções de teste para o Multitasking */
void task_a(void) {
    asm volatile("sti"); 
    int contador = 0;
    while (contador < 4) {
        log_info("Processo A rodando...");
        contador++;
        yield();
        for (volatile int i = 0; i < 10000000; i++); 
    }
    log_info("Processo A finalizou.");
    
    // IMPORTANTE: Continua cedendo a vez para o B poder terminar
    while(1) {
        yield();
    }
}

void task_b(void) {
    asm volatile("sti"); 
    int contador = 0;
    while (contador < 5) {
        log_info("Processo B rodando...");
        contador++;
        yield();
        for (volatile int i = 0; i < 10000000; i++); 
    }
    log_info("Processo B finalizou.");
    
    // IMPORTANTE: Continua cedendo a vez para o A (caso ele ainda precise)
    while(1) {
        yield();
    }
}

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

    /* 1. Criamos os processos PRIMEIRO */
    //process_create_kernel("taskA", task_a);
    //process_create_kernel("taskB", task_b);

    shell_run();

    /* 3. O kmain (PID 0) vai dormir, deixando A e B lutarem pelo CPU */
    while (1) {
        asm volatile("hlt");
    }
}