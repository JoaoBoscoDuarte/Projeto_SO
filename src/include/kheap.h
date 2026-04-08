#ifndef KHEAP_H
#define KHEAP_H

void kheap_init(void);
void *kmalloc(unsigned int size);
void kfree(void *ptr);
unsigned int kheap_used_bytes(void);
unsigned int kheap_total_bytes(void);

#endif