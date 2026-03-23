#include "kheap.h"
#include "paging.h"
#include "pfa.h"

#define KHEAP_START 0xC1000000

typedef struct block {
    unsigned int size;
    int free;
    struct block *next;
} block_t;

static block_t *heap_head = 0;
static unsigned int heap_end = KHEAP_START;

static void heap_expand()
{
    unsigned int phys = pfa_alloc();
    if (!phys) return;

    paging_map(heap_end, phys, PAGE_KERNEL);

    // zera a página
    unsigned int *ptr = (unsigned int *)heap_end;
    for (int i = 0; i < 1024; i++)
        ptr[i] = 0;

    heap_end += PAGE_SIZE;
}

void kheap_init(void)
{
    heap_expand();

    heap_head = (block_t *)KHEAP_START;
    heap_head->size = PAGE_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = 0;
}

void *kmalloc(unsigned int size)
{
    block_t *curr = heap_head;

    while (curr) {
        if (curr->free && curr->size >= size) {
            curr->free = 0;
            return (void *)(curr + 1);
        }

        if (!curr->next) break;
        curr = curr->next;
    }

    // precisa expandir
    heap_expand();

    block_t *new_block = (block_t *)(heap_end - PAGE_SIZE);
    new_block->size = PAGE_SIZE - sizeof(block_t);
    new_block->free = 0;
    new_block->next = 0;

    curr->next = new_block;

    return (void *)(new_block + 1);
}

void kfree(void *ptr)
{
    if (!ptr) return;

    block_t *block = ((block_t *)ptr) - 1;
    block->free = 1;
}