// ============================================================================
// KEYBOARD_BR.C - Mapeamento de teclado brasileiro ABNT2
// ============================================================================
// Suporta caracteres acentuados usando Code Page 437
// ============================================================================

#include "printf.h"
#include "fb.h"
extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);

// ============================================================================
// Code Page 437 - Caracteres especiais
// ============================================================================
// Alguns caracteres úteis da CP437:
// 0x80 = Ç    0x81 = ü    0x82 = é    0x83 = â    0x84 = ä
// 0x85 = à    0x86 = å    0x87 = ç    0x88 = ê    0x89 = ë
// 0x8A = è    0x8B = ï    0x8C = î    0x8D = ì    0x8E = Ä
// 0x8F = Å    0x90 = É    0x91 = æ    0x92 = Æ    0x93 = ô
// 0x94 = ö    0x95 = ò    0x96 = û    0x97 = ù    0x98 = ÿ
// 0x99 = Ö    0x9A = Ü    0x9B = ¢    0x9C = £    0x9D = ¥
// 0xA0 = á    0xA1 = í    0xA2 = ó    0xA3 = ú    0xA4 = ñ
// 0xA5 = Ñ    0xE1 = ß    0xE6 = µ

// Mapa básico do teclado brasileiro (sem shift)
unsigned char kbd_br_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0xA0, '[', '\n',  // 0xA0 = á
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x87, '~', '\'',        // 0x87 = ç
    0,  ']', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', ';',   
    0,  '*', 0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Mapa com shift pressionado
unsigned char kbd_br_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '&', '*', '(', ')', '_', '+', '\b',   
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0x80, '^', '"',        // 0x80 = Ç
    0,  '}', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', ':', 
    0,  '*', 0,  ' '
};

// Flags de estado do teclado
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

// Variável global para rastrear a posição
static int terminal_pos = 0; 

// ============================================================================
// keyboard_handler_br - Handler de interrupção do teclado brasileiro
// ============================================================================
void keyboard_handler_br(void) {
    unsigned char scancode = inb(0x60);
    
    // Verifica se é key press (bit 7 = 0) ou key release (bit 7 = 1)
    int key_released = scancode & 0x80;
    scancode &= 0x7F;  // Remove o bit de release
    
    // Detecta teclas modificadoras
    if (scancode == 0x2A || scancode == 0x36) {  // Left/Right Shift
        shift_pressed = !key_released;
        return;
    }
    if (scancode == 0x1D) {  // Ctrl
        ctrl_pressed = !key_released;
        return;
    }
    if (scancode == 0x38) {  // Alt
        alt_pressed = !key_released;
        return;
    }
    
    // Ignora key release de teclas normais
    if (key_released) return;
    
    // Seleciona o mapa correto
    char c = shift_pressed ? kbd_br_map_shift[scancode] : kbd_br_map[scancode];
    
    if (c == '\b') {
        // Backspace
        if (terminal_pos > 0) { 
            terminal_pos--;
            fb_write_cell(terminal_pos, ' ', 0x07, 0x00);
        }
    } 
    else if (c == '\n') {
        // Enter
        terminal_pos = (terminal_pos / 80 + 1) * 80;
    }
    else if (c != 0) {
        // Caractere normal (incluindo acentuados)
        fb_write_cell(terminal_pos, c, 0x0F, 0x00);
        terminal_pos++;
    }

    // Scroll se necessário
    if (terminal_pos >= 80 * 25) {
        fb_scroll();
        terminal_pos = 80 * 24;
    }
    fb_move_cursor(terminal_pos);
}
