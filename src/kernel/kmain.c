#include "fb.h"
#include "serial.h"
#include "printf.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"

void kmain(void) {
    fb_clear(); // Agora a tela começa limpa e preta
    
    gdt_init();
    idt_init();
    pic_remap();

    kprintf(OUTPUT_FB, "Kernel inicializado com sucesso!\n");
    kprintf(OUTPUT_FB, "Digite algo:\n");

    asm volatile("sti");

    while(1) {
        asm volatile("hlt");
    }
}
