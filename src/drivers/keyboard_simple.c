// ============================================================================
// KEYBOARD_SIMPLE.C - Handler de teclado com suporte a acentos
// ============================================================================

#include "printf.h"
#include "fb.h"

extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);

// Mapa básico do teclado (sem shift)
unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0,  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   
    0,  '*', 0,  ' '
};

// Mapa com shift
unsigned char kbd_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',   
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',     
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   
    0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   
    0,  '*', 0,  ' '
};

// Tabela de acentuação: tecla morta + letra = caractere acentuado (CP437)
// Formato: {tecla_morta, letra_base, resultado_cp437}
struct accent_map {
    char dead_key;
    char base;
    unsigned char result;
};

// Tabela de acentos
struct accent_map accents[] = {
    // Acento agudo (')
    {'\'', 'a', 0xA0},  // á
    {'\'', 'e', 0x82},  // é
    {'\'', 'i', 0xA1},  // í
    {'\'', 'o', 0xA2},  // ó
    {'\'', 'u', 0xA3},  // ú
    {'\'', 'A', 0xB5},  // Á
    {'\'', 'E', 0x90},  // É
    {'\'', 'I', 0xD6},  // Í
    {'\'', 'O', 0xE0},  // Ó
    {'\'', 'U', 0xE9},  // Ú
    
    // Til (~)
    {'~', 'a', 0xC6},   // ã
    {'~', 'o', 0xE4},   // õ
    {'~', 'A', 0xC7},   // Ã
    {'~', 'O', 0xE5},   // Õ
    
    // Circunflexo (^)
    {'^', 'a', 0x83},   // â
    {'^', 'e', 0x88},   // ê
    {'^', 'o', 0x93},   // ô
    {'^', 'A', 0xB6},   // Â
    {'^', 'E', 0xD2},   // Ê
    {'^', 'O', 0xE2},   // Ô
    
    // Cedilha (,)
    {',', 'c', 0x87},   // ç
    {',', 'C', 0x80},   // Ç
    
    {0, 0, 0}  // Fim
};

static int shift_pressed = 0;
static int terminal_pos = 0;
static char dead_key = 0;  // Armazena tecla morta pendente

void keyboard_handler_c(void) {
    unsigned char scancode = inb(0x60);
    
    // Detecta key release
    int key_released = scancode & 0x80;
    scancode &= 0x7F;
    
    // Shift
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = !key_released;
        return;
    }
    
    if (key_released) return;
    
    // Pega caractere
    char c = shift_pressed ? kbd_map_shift[scancode] : kbd_map[scancode];
    
    if (c == '\b') {
        // Backspace
        if (terminal_pos > 0) {
            terminal_pos--;
            fb_write_cell(terminal_pos, ' ', 0x0F, 0x00);
        }
        dead_key = 0;  // Cancela tecla morta
    }
    else if (c == '\n') {
        // Enter
        terminal_pos = (terminal_pos / 80 + 1) * 80;
        dead_key = 0;
    }
    else if (c != 0) {
        // Se há tecla morta pendente, tenta acentuar
        if (dead_key != 0) {
            int found = 0;
            for (int i = 0; accents[i].dead_key != 0; i++) {
                if (accents[i].dead_key == dead_key && accents[i].base == c) {
                    // Encontrou! Escreve caractere acentuado
                    fb_write_cell(terminal_pos, accents[i].result, 0x0F, 0x00);
                    terminal_pos++;
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                // Não encontrou combinação, escreve ambos
                fb_write_cell(terminal_pos, dead_key, 0x0F, 0x00);
                terminal_pos++;
                fb_write_cell(terminal_pos, c, 0x0F, 0x00);
                terminal_pos++;
            }
            
            dead_key = 0;
        }
        // Verifica se é tecla morta
        else if (c == '\'' || c == '~' || c == '^' || c == ',') {
            dead_key = c;  // Armazena e espera próxima tecla
        }
        else {
            // Caractere normal
            fb_write_cell(terminal_pos, c, 0x0F, 0x00);
            terminal_pos++;
        }
    }
    
    // Scroll
    if (terminal_pos >= 80 * 25) {
        fb_scroll();
        terminal_pos = 80 * 24;
    }
    fb_move_cursor(terminal_pos);
}
