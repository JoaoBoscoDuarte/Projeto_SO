#include "screen.h"

extern void idt_install(void);

void kmain(void)
{
    kprint("kmain entrou!\n");

    idt_install();
    kprint("IDT ok (carregada)\n");

    kprint("Testando int $0...\n");
    __asm__ volatile("int $0");

    kprint("ERRO: voltou do int $0\n");
    for(;;) __asm__ volatile("hlt");
}