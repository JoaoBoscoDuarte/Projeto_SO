#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "multiboot.h"

void kmain(unsigned int ebx) {
    multiboot_info_t *mbinfo = (multiboot_info_t *) ebx;
    unsigned int address_of_module = mbinfo->mods_addr;

    unsigned int number_of_modules = mbinfo->mods_count;
    unsigned int flags = mbinfo->flags;
    
    fb_clear(); // Agora a tela começa limpa e preta

    kprintf(OUTPUT_FB, "\nFlags: 0x%x\n", flags);
    kprintf(OUTPUT_FB, "Endereco do modulo: 0x%x\n", address_of_module);
    kprintf(OUTPUT_FB, "Numero de modulos: %u\n", number_of_modules);
    
    gdt_init();
    serial_init();
    idt_init();
    pic_remap();

    // 4. Exibe mensagem de inicialização
    // log_info envia para framebuffer E serial
    log_info("Sistema Operacional Iniciado");

    // 5. Log de debug (apenas serial, não aparece na tela)
    // Útil para debug sem poluir a tela
    log_debug("Drivers carregados com sucesso");
    
    // 6. Testa kprintf com formatação
    // OUTPUT_FB: envia apenas para framebuffer (tela)
    // %d: formata inteiro decimal
    kprintf(OUTPUT_FB, "Bem-vindo! Valor: %d\n", 42);
    
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