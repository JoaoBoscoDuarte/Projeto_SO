#include "kheap.h"
#include "paging.h"
#include "pfa.h"

/*
 * kheap.c — Alocador de heap do kernel (free-list com split e coalescing)
 *
 * A heap cresce sob demanda a partir de KHEAP_START (0xC1000000).
 * Cada expansão aloca um page frame via pfa_alloc() e o mapeia via paging_map().
 *
 * Layout de cada bloco:
 *   [ block_t header | dados do usuário ... ]
 *
 * A lista encadeada de blocos é mantida em ordem crescente de endereço,
 * o que permite o coalescing simples de blocos adjacentes em kfree().
 */

#define KHEAP_START 0xC1000000u

/*
 * Threshold mínimo para fazer split de um bloco livre.
 * Se o bloco restante seria menor que isso, não vale fazer split.
 */
#define SPLIT_THRESHOLD (sizeof(block_t) + 16u)

typedef struct block {
    unsigned int   size;  /* tamanho dos dados (sem contar o header) */
    int            free;  /* 1 = livre, 0 = ocupado */
    struct block  *next;  /* próximo bloco na lista */
} block_t;

static block_t   *heap_head = (block_t *)0;
static unsigned int heap_end = KHEAP_START;

/* -------------------------------------------------------------------------
 * heap_expand — aloca uma página física e mapeia em heap_end
 *
 * Retorna 0 em sucesso, -1 em falha (pfa_alloc sem frames ou paging falhou).
 * ------------------------------------------------------------------------ */
static int heap_expand(void)
{
    unsigned int phys = pfa_alloc();
    if (phys == 0)
        return -1;

    if (paging_map(heap_end, phys, PAGE_KERNEL) != 0) {
        pfa_free(phys);
        return -1;
    }

    /* Zera a nova página para evitar lixo nos headers de bloco */
    unsigned int *ptr = (unsigned int *)heap_end;
    unsigned int i;
    for (i = 0; i < PAGE_SIZE / sizeof(unsigned int); i++)
        ptr[i] = 0;

    heap_end += PAGE_SIZE;
    return 0;
}

/* -------------------------------------------------------------------------
 * kheap_init — inicializa a heap com a primeira página
 * ------------------------------------------------------------------------ */
void kheap_init(void)
{
    if (heap_expand() != 0)
        return; /* sem memória — heap permanece inativa */

    heap_head = (block_t *)KHEAP_START;
    heap_head->size = PAGE_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = (block_t *)0;
}

/* -------------------------------------------------------------------------
 * kmalloc — aloca 'size' bytes da heap do kernel
 *
 * Estratégia: first-fit com block splitting.
 * Retorna NULL em caso de falha de alocação.
 * ------------------------------------------------------------------------ */
void *kmalloc(unsigned int size)
{
    block_t *curr;

    if (size == 0)
        return (void *)0;

    /* Alinha o tamanho a 4 bytes */
    size = (size + 3u) & ~3u;

    curr = heap_head;
    while (curr) {
        if (curr->free && curr->size >= size) {
            /* Tenta fazer split se o bloco restante for grande o suficiente */
            if (curr->size >= size + SPLIT_THRESHOLD) {
                block_t *split = (block_t *)((unsigned char *)(curr + 1) + size);
                split->size = curr->size - size - sizeof(block_t);
                split->free = 1;
                split->next = curr->next;

                curr->size = size;
                curr->next = split;
            }
            curr->free = 0;
            return (void *)(curr + 1);
        }

        if (!curr->next)
            break;

        curr = curr->next;
    }

    /* Nenhum bloco livre adequado — expande a heap */
    unsigned int old_end = heap_end;
    if (heap_expand() != 0)
        return (void *)0; /* falha crítica: sem frames */

    /* Cria um novo bloco na página recém-mapeada */
    block_t *new_block = (block_t *)old_end;
    new_block->size = PAGE_SIZE - sizeof(block_t);
    new_block->free = 1;
    new_block->next = (block_t *)0;

    /* Encadeia no final da lista */
    if (curr)
        curr->next = new_block;
    else
        heap_head = new_block;

    /* Tenta split e marca como ocupado */
    if (new_block->size >= size + SPLIT_THRESHOLD) {
        block_t *split = (block_t *)((unsigned char *)(new_block + 1) + size);
        split->size = new_block->size - size - sizeof(block_t);
        split->free = 1;
        split->next = new_block->next;

        new_block->size = size;
        new_block->next = split;
    }
    new_block->free = 0;
    return (void *)(new_block + 1);
}

/* -------------------------------------------------------------------------
 * kfree — libera um bloco alocado por kmalloc
 *
 * Faz coalescing com o bloco seguinte se ambos estiverem livres.
 * ------------------------------------------------------------------------ */
void kfree(void *ptr)
{
    block_t *curr;

    if (!ptr)
        return;

    block_t *block = ((block_t *)ptr) - 1;
    block->free = 1;

    /* Coalescing: funde com o próximo bloco se também estiver livre */
    curr = block;
    while (curr->next && curr->next->free) {
        curr->size += sizeof(block_t) + curr->next->size;
        curr->next  = curr->next->next;
    }
}

unsigned int kheap_used_bytes(void)
{
    block_t *curr = heap_head;
    unsigned int used = 0;
    while (curr) {
        if (!curr->free)
            used += curr->size;
        curr = curr->next;
    }
    return used;
}

unsigned int kheap_total_bytes(void)
{
    return heap_end - KHEAP_START;
}
