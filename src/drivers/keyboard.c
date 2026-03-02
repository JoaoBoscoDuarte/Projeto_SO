// Usei os mesmos includes que vi no seu kmain.c
#include "printf.h"
#include "fb.h" // Se precisar
extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);

// Mapa básico do teclado americano (Scancode para ASCII)
unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0,  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   
    0,  '*', 0,  ' '
};

// Variável global para rastrear a posição no keyboard.c ou use a do fb.c
static int terminal_pos = 80 * 3; 

void keyboard_handler_c(void) {
    unsigned char scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        char c = kbd_map[scancode];
        
        if (c == '\b') {
            // Só apaga se não for o início da nossa linha de comando (ex: > )
            if (terminal_pos > 80 * 3) { 
                terminal_pos--;
                fb_write_cell(terminal_pos, ' ', 0x07, 0x00); // Apaga com fundo preto
            }
        } 
        else if (c == '\n') {
            terminal_pos = (terminal_pos / 80 + 1) * 80;
        }
        else if (c != 0) {
            fb_write_cell(terminal_pos, c, 0x0F, 0x00); // Branco no Preto
            terminal_pos++;
        }

        if (terminal_pos >= 80 * 25) {
            fb_scroll();
            terminal_pos = 80 * 24; // Volta para o início da última linha
        }
        fb_move_cursor(terminal_pos);
    }
}