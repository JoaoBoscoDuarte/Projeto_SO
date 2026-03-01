#ifndef INCLUDE_GDT_H
#define INCLUDE_GDT_H

// É boa prática ter as structs no header para uso global
struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

void gdt_init(void);
void gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran);

// Avise o C que gdt_flush está no assembly
extern void gdt_flush(unsigned int gp_ptr); 

#endif