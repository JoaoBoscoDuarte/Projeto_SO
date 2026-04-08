#include "pic.h"
#include "io.h" // Certifique-se de que outb/inb estão acessíveis

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

void pic_remap(void) {
    // 1. Inicia a sequência de inicialização (ICW1)
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // 2. Remapeia os offsets (ICW2)
    // O PIC1 começa em 32 (0x20) e o PIC2 em 40 (0x28)
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // 3. Configura a cascata (ICW3)
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // 4. Modo 8086 (ICW4)
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // 5. MÁSCARAS DE INTERRUPÇÃO (O ponto crucial!)
    // 0xFC = 11111100 — habilita IRQ0 (timer) e IRQ1 (teclado)
    outb(PIC1_DATA, 0xFC); 
    
    // Desativa todas as interrupções do PIC escravo
    outb(PIC2_DATA, 0xFF); 
}