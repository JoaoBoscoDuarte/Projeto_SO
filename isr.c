#include "isr.h"
#include "screen.h"

static const char* exc_msg[32] = {
    "Division By Zero", "Debug", "NMI", "Breakpoint", "Overflow",
    "Bound Range", "Invalid Opcode", "Device Not Available", "Double Fault",
    "Coprocessor Segment", "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
    "General Protection", "Page Fault", "Reserved",
    "x87 FP", "Alignment Check", "Machine Check", "SIMD FP",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor", "VMM Communication", "Security", "Reserved"
};

void isr_handler(regs_t* r)
{
    kprint("\n=== EXCEPTION ===\n");
    if (r->int_no < 32) {
        kprint("Tipo: ");
        kprint(exc_msg[r->int_no]);
        kprint("\n");
    }
    kprint("CPU Exception! Travando.\n");
    for(;;) __asm__ volatile("hlt");
}