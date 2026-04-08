#ifndef INCLUDE_PFA_H
#define INCLUDE_PFA_H

#include "multiboot.h"

void         pfa_init(unsigned int kphys_start, unsigned int kphys_end, multiboot_info_t *mbinfo);
unsigned int pfa_alloc(void);
void         pfa_free(unsigned int addr);
unsigned int pfa_total_frames(void);
unsigned int pfa_used_frames(void);
unsigned int pfa_free_frames(void);
unsigned int pfa_bitmap_start(void);
unsigned int pfa_bitmap_end(void);

#endif