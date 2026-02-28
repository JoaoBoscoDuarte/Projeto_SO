bits 32
section .text

global idt_load
extern idtp

idt_load:
    lidt [idtp]
    ret