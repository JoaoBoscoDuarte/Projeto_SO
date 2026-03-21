#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "multiboot.h"
#include "pfa.h"

/* Ponteiro para função de entrada de um módulo carregado pelo GRUB (sem argumentos, não retorna). */
typedef void (*call_module_t)(void);

void kmain(unsigned int multiboot_addr,
           unsigned int kernel_virtual_start,
           unsigned int kernel_virtual_end,
           unsigned int kernel_physical_start,
           unsigned int kernel_physical_end) 
{
    multiboot_info_t *mbinfo = (multiboot_info_t *) multiboot_addr;

    unsigned int number_of_modules = mbinfo->mods_count;
    unsigned int flags = mbinfo->flags;
    
    fb_clear(); // Agora a tela começa limpa e preta
    pfa_init(kernel_physical_start, kernel_physical_end, mbinfo);
    gdt_init();
    serial_init();
    idt_init();
    pic_remap();

    // 4. Exibe mensagem de inicialização
    log_info("Sistema Operacional Iniciado");

        // Mostra informações sobre endereços de memória do kernel
    kprintf(OUTPUT_FB, "kernel virtual: 0x%x - 0x%x\n", kernel_virtual_start, kernel_virtual_end);
    kprintf(OUTPUT_FB, "kernel physical: 0x%x - 0x%x\n", kernel_physical_start, kernel_physical_end);
    kprintf(OUTPUT_FB, "Tamanho do kernel: %dKB\n", (kernel_virtual_end - kernel_virtual_start) / 1024);


    // Mostra 3 frames alocados e 1 liberado
    unsigned int f1 = pfa_alloc();
    unsigned int f2 = pfa_alloc();
    unsigned int f3 = pfa_alloc();
    kprintf(OUTPUT_FB, "frame1: 0x%x\n", f1);
    kprintf(OUTPUT_FB, "frame2: 0x%x\n", f2);
    kprintf(OUTPUT_FB, "frame3: 0x%x\n", f3);
    pfa_free(f1);
    unsigned int f4 = pfa_alloc(); /* deve reutilizar f1 */
    kprintf(OUTPUT_FB, "frame4: 0x%x\n", f4);

    kprintf(OUTPUT_FB, "bitwise AND result: %d\n", (flags & MULTIBOOT_INFO_MODS) && number_of_modules > 0);
    kprintf(OUTPUT_FB, "\nFlags: %d\n", flags);
    kprintf(OUTPUT_FB, "Endereco da lista de modulos: 0x%x\n", (unsigned int) mbinfo->mods_addr);
    kprintf(OUTPUT_FB, "Numero de modulos: %d\n", number_of_modules);
    // Chama o primeiro módulo carregado pelo GRUB, se existir
    if ((flags & MULTIBOOT_INFO_MODS) && number_of_modules > 0) {
        kprintf(OUTPUT_FB, "Carregando modulo...\n");
        multiboot_module_t *mods = (multiboot_module_t *) mbinfo->mods_addr;
        unsigned int module_entry = mods[0].mod_start;  /* início do código do módulo = ponto de entrada */
        call_module_t start_program = (call_module_t) module_entry;
        start_program();
        /* só chegamos aqui se o módulo retornar (ex.: program.s faz jmp $ e não retorna) */
    }


    // 5. Log de debug (apenas serial, não aparece na tela)
    // Útil para debug sem poluir a tela
    log_debug("Drivers carregados com sucesso");
    
    // 6. Testa kprintf com formatação
    // OUTPUT_FB: envia apenas para framebuffer (tela)
    // %d: formata inteiro decimal
    
    // OUTPUT_SERIAL: envia apenas para porta serial
    // %x: formata em hexadecimal
    kprintf(OUTPUT_SERIAL, "Endereco: 0x%x\n", 0xB8000);
    
    // 7. Testa mensagem de erro
    log_error("Teste de mensagem de erro");

    asm volatile("sti");

    while(1) {
        asm volatile("hlt");
    }
}