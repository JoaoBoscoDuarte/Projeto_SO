#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"

void kmain(void) {
    fb_clear(); // Agora a tela começa limpa e preta
    
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