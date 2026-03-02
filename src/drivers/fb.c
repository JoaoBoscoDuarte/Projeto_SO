#include "io.h"
#include "fb.h"

// ============================================================================
// FRAMEBUFFER - Driver de vídeo VGA em modo texto
// ============================================================================
// Memória VGA: 0xB8000 - 0xB8FA0 (4000 bytes)
// Tela: 80 colunas x 25 linhas = 2000 caracteres
// Cada caractere ocupa 2 bytes: [caractere][atributo de cor]
// ============================================================================

// Portas de I/O do controlador VGA para controle do cursor
#define FB_COMMAND_PORT 0x3D4   // Porta de comando (seleciona registrador)
#define FB_DATA_PORT    0x3D5   // Porta de dados (lê/escreve valor)
#define FB_HIGH_BYTE    14      // Registrador: byte alto da posição do cursor
#define FB_LOW_BYTE     15      // Registrador: byte baixo da posição do cursor

// Ponteiro para a memória de vídeo VGA
static char *fb = (char *) 0x000B8000;  // Endereço fixo da memória VGA

// Posição atual do cursor (0-1999, onde 0 = canto superior esquerdo)
static unsigned short cursor_pos = 0;

// ============================================================================
// fb_clear - Limpa a tela inteira
// ============================================================================
// Preenche toda a tela com espaços em branco com fundo preto
void fb_clear(void)
{
    unsigned int i;
    // Percorre todas as 2000 células da tela (80x25)
    for (i = 0; i < 80 * 25; i++) {
        fb[i * 2] = ' ';        // Byte 0: caractere espaço
        fb[i * 2 + 1] = ((FB_BLACK & 0x0F) << 4) | (FB_WHITE & 0x0F);
                                // Byte 1: atributo de cor
                                // Bits 7-4: cor de fundo (preto)
                                // Bits 3-0: cor do texto (branco)
    }
    cursor_pos = 0;             // Reseta cursor para início
    fb_move_cursor(cursor_pos); // Atualiza cursor no hardware
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
    fb[i * 2] = c;              // Escreve o caractere
    fb[i * 2 + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
                                // Combina cores: fundo nos 4 bits altos,
                                // texto nos 4 bits baixos
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
