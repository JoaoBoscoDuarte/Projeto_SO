#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"

void kmain(void)
{
    /* 2. Inicialize a segmentação antes dos drivers.
       Isso configura os seletores cs, ds, ss, etc. */
    gdt_init();

    serial_init();
    fb_clear();
    
    log_info("Sistema Operacional Iniciado");

    /* Agora o processador está operando com os segmentos 
       definidos: 0x08 para código e 0x10 para dados */
    log_debug("Drivers carregados com sucesso");
    
    kprintf(OUTPUT_FB, "Bem-vindo! Valor: %d\n", 42);
    kprintf(OUTPUT_SERIAL, "Endereco: 0x%x\n", 0xB8000);
    
    log_error("Teste de mensagem de erro");
    
    while(1);
}
