#include "fb.h"
#include "serial.h"
#include "printf.h"

void kmain(void)
{
    serial_init();
    fb_clear();
    
    log_info("Sistema Operacional Iniciado");
    log_debug("Drivers carregados com sucesso");
    
    kprintf(OUTPUT_FB, "Bem-vindo! Valor: %d\n", 42);
    kprintf(OUTPUT_SERIAL, "Endereco: 0x%x\n", 0xB8000);
    
    log_error("Teste de mensagem de erro");
    
    while(1);
}
