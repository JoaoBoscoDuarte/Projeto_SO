#ifndef KHEAP_H
#define KHEAP_H

void kheap_init(void);
void *kmalloc(unsigned int size);
void kfree(void *ptr);

#endif