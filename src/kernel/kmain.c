#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"

// ============================================================================
// kmain - Função principal do kernel
// ============================================================================

void kmain(void)
{
    // 1. Inicializa a GDT (Global Descriptor Table)
    gdt_init();

    // 2. Inicializa a porta serial (COM1)
    // Usada para debug: envia logs para arquivo no Bochs
    serial_init();
    
    // 3. Limpa a tela (preenche com espaços pretos)
    // Prepara o framebuffer VGA para exibir texto
    fb_clear();
    
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
    
    // 8. Loop infinito
    // Mantém o kernel rodando (não deve retornar)
    while(1);
}
