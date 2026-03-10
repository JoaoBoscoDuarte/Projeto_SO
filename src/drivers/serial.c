#include "io.h"
#include "serial.h"

// ============================================================================
// SERIAL - Driver da porta serial (COM1)
// ============================================================================
// Porta serial é usada para debug: envia logs para arquivo no Bochs.
// Mais confiável que framebuffer para debug de baixo nível.
// ============================================================================

// Macros para calcular endereços das portas (COM1 = 0x3F8)
#define SERIAL_DATA_PORT(base)          (base)      // +0: dados
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)  // +2: FIFO
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)  // +3: configuração
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)  // +4: modem
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)  // +5: status

#define SERIAL_LINE_ENABLE_DLAB 0x80    // Bit para acessar divisor de baud rate

// ============================================================================
// serial_configure_baud_rate - Configura velocidade de transmissão
// ============================================================================
// com: porta base (ex: 0x3F8 para COM1)
// divisor: divisor do clock (3 = 38400 baud, 1 = 115200 baud)
void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    // Habilita DLAB (Divisor Latch Access Bit) para acessar registradores do divisor
    outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
    
    // Envia byte baixo do divisor
    outb(SERIAL_DATA_PORT(com), divisor & 0x00FF);
    
    // Envia byte alto do divisor
    outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
}

// ============================================================================
// serial_configure_line - Configura formato dos dados
// ============================================================================
// 0x03 = 00000011b
//   Bits 1-0: 11 = 8 bits por caractere
//   Bit 2: 0 = 1 stop bit
//   Bits 5-3: 000 = sem paridade
//   Bit 7: 0 = desabilita DLAB
void serial_configure_line(unsigned short com)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

// ============================================================================
// serial_configure_buffers - Configura FIFO (buffer de transmissão)
// ============================================================================
// 0xC7 = 11000111b
//   Bit 0: 1 = habilita FIFO
//   Bit 1: 1 = limpa FIFO de recepção
//   Bit 2: 1 = limpa FIFO de transmissão
//   Bits 7-6: 11 = trigger level de 14 bytes
void serial_configure_buffers(unsigned short com)
{
    outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

// ============================================================================
// serial_configure_modem - Configura sinais de controle do modem
// ============================================================================
// 0x03 = 00000011b
//   Bit 0: 1 = Data Terminal Ready (DTR)
//   Bit 1: 1 = Request To Send (RTS)
void serial_configure_modem(unsigned short com)
{
    outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

// ============================================================================
// serial_is_transmit_fifo_empty - Verifica se pode enviar dados
// ============================================================================
// Lê o registrador de status e verifica bit 5 (Transmit Empty)
// Retorna: 1 se pode enviar, 0 se deve esperar
int serial_is_transmit_fifo_empty(unsigned int com)
{
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;  // Bit 5
}

// ============================================================================
// serial_write - Envia dados pela porta serial
// ============================================================================
// com: porta base
// buf: buffer com dados a enviar
// len: quantidade de bytes
void serial_write(unsigned short com, char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        // Espera até o buffer estar vazio
        while (!serial_is_transmit_fifo_empty(com));
        
        // Envia o byte
        outb(SERIAL_DATA_PORT(com), buf[i]);
    }
}

// ============================================================================
// serial_init - Inicializa a porta serial COM1
// ============================================================================
void serial_init(void)
{
    serial_configure_baud_rate(SERIAL_COM1_BASE, 3);    // 38400 baud
    serial_configure_line(SERIAL_COM1_BASE);            // 8N1
    serial_configure_buffers(SERIAL_COM1_BASE);         // FIFO habilitado
    serial_configure_modem(SERIAL_COM1_BASE);           // DTR/RTS ativos
}
