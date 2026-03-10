#include "io.h"
#include "fb.h"

// ============================================================================
// FRAMEBUFFER - Driver de vídeo VGA em modo texto
// ============================================================================
// Memória VGA: 0xB8000 - 0xB8FA0 (4000 bytes)
// Tela: 80 colunas x 25 linhas = 2000 caracteres
// Cada caractere ocupa 2 bytes: [caractere][atributo de cor]
// ============================================================================

// static char *fb = (char *) 0x000B8000;
static unsigned short cursor_pos = 0;

void fb_clear(void) {
    unsigned char *mem_vga = (unsigned char *) 0x000B8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        mem_vga[i] = ' ';      // Espaço vazio
        mem_vga[i+1] = 0x07;   // 0x07 é Cinza Claro no Preto (Legível e padrão)
    }
}

// ============================================================================
// fb_write_cell - Escreve um caractere em uma posição específica
// ============================================================================
// i: posição na tela (0-1999)
// c: caractere a escrever
// fg: cor do texto (foreground)
// bg: cor do fundo (background)
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    unsigned char *mem_vga = (unsigned char *) 0x000B8000; // Force o ponteiro aqui
    mem_vga[i * 2] = c;
    mem_vga[i * 2 + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

// ============================================================================
// fb_move_cursor - Move o cursor piscante na tela
// ============================================================================
// pos: nova posição do cursor (0-1999)
// Comunica com o controlador VGA via portas I/O
void fb_move_cursor(unsigned short pos)
{
    // Envia byte alto da posição (bits 15-8)
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE);        // Seleciona registrador 14
    outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));  // Envia bits 15-8
    
    // Envia byte baixo da posição (bits 7-0)
    outb(FB_COMMAND_PORT, FB_LOW_BYTE);         // Seleciona registrador 15
    outb(FB_DATA_PORT, pos & 0x00FF);           // Envia bits 7-0
}

// ============================================================================
// fb_write - Escreve uma string na tela
// ============================================================================
// buf: buffer com os caracteres a escrever
// len: quantidade de caracteres
// Retorna: número de caracteres escritos
int fb_write(char *buf, unsigned int len)
{
    unsigned int i;
    
    // Processa cada caractere do buffer
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            // Nova linha: move cursor para início da próxima linha
            // Divide por 80 para obter linha atual, soma 1, multiplica por 80
            cursor_pos = (cursor_pos / 80 + 1) * 80;
            
        } else if (buf[i] == '\r') {
            // Carriage return: volta para início da linha atual
            cursor_pos = (cursor_pos / 80) * 80;
            
        } else {
            // Caractere normal: escreve e avança cursor
            fb_write_cell(cursor_pos, buf[i], FB_WHITE, FB_BLACK);
            cursor_pos++;
        }
        
        // Se cursor passou do fim da tela, volta para o início
        // (implementação simples, sem scroll)
        if (cursor_pos >= 80 * 25) {
            cursor_pos = 0;
        }
    }
    
    // Atualiza posição do cursor no hardware
    fb_move_cursor(cursor_pos);
    return (int)len;
}

void fb_put_char(unsigned int row, unsigned int col, char c, unsigned char fg, unsigned char bg) {
    unsigned char *fb = (unsigned char *) 0xB8000;
    unsigned int index = 2 * (row * 80 + col);
    fb[index] = c;           // Byte 0: Caractere
    fb[index + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F); // Byte 1: Cor
}

void fb_scroll(void) {
    unsigned char *mem_vga = (unsigned char *) 0x000B8000;
    for (int i = 0; i < 80 * 24 * 2; i++) {
        mem_vga[i] = mem_vga[i + 80 * 2];
    }
    for (int i = 80 * 24 * 2; i < 80 * 25 * 2; i += 2) {
        mem_vga[i] = ' ';
        mem_vga[i+1] = 0x07; // Fundo preto
    }
}