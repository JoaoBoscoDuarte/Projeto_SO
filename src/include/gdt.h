#ifndef INCLUDE_GDT_H
#define INCLUDE_GDT_H

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

/* Segment selector offsets (index * 8) */
#define GDT_KERNEL_CODE  0x08
#define GDT_KERNEL_DATA  0x10
#define GDT_USER_CODE    0x18
#define GDT_USER_DATA    0x20
#define GDT_TSS          0x28

/* Selectors with RPL=3 for use in iret (Capítulo 11) */
#define SEL_USER_CODE    (GDT_USER_CODE | 0x3)   /* 0x1B */
#define SEL_USER_DATA    (GDT_USER_DATA | 0x3)   /* 0x23 */

/* TSS selector: RPL=3 required by ltr when DPL=0 on the descriptor is used */
#define SEL_TSS          (GDT_TSS | 0x3)         /* 0x2B */

void gdt_init(void);
void gdt_set_gate(int num, unsigned int base, unsigned int limit,
                  unsigned char access, unsigned char gran);

extern void gdt_flush(unsigned int gp_ptr);

#endif