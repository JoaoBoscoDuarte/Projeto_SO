# Implementação de Gerenciamento Dinâmico de Memória do Kernel

## Resumo da Mudança (O que foi modificado)

Foram realizadas modificações no subsistema de memória do kernel para substituir uma abordagem estática por uma arquitetura dinâmica baseada em três camadas: Page Frame Allocator (PFA), paging com page tables de 4KB e kernel heap.

### Arquivos alterados

- src/drivers/pfa.c  
- src/include/pfa.h  
- src/drivers/paging.c  
- src/include/paging.h  
- src/kernel/kheap.c  
- src/include/kheap.h  
- src/kernel/kmain.c  
- Makefile  

---

## Objetivo (Para que)

Implementar um sistema completo de gerenciamento de memória no kernel, permitindo:

- descoberta dinâmica de memória via Multiboot (mmap)
- alocação de frames físicos de 4KB
- mapeamento virtual-físico via paging
- alocação dinâmica de memória em bytes (heap)

---

## Justificativa (Por que)

A versão anterior:

- assumia memória fixa (MAX_FRAMES)
- utilizava bitmap estático
- não possuía heap

Isso limitava o kernel e não refletia o comportamento real do hardware.

A mudança foi necessária para:

- remover hardcode de memória
- evitar uso incorreto de regiões críticas
- permitir alocação dinâmica
- preparar o kernel para evoluções futuras

---

## Detalhes Técnicos

### 1. Page Frame Allocator (PFA)

#### Nova lógica

1. Percorre mmap do Multiboot
2. Calcula total de frames
3. Cria bitmap dinâmico
4. Marca tudo como ocupado
5. Libera apenas regiões AVAILABLE
6. Re-reserva regiões críticas

#### Fórmulas

total_frames = align_up(max_phys) / PAGE_SIZE  
bitmap_words = (total_frames + 31) / 32  

#### Complexidade

- init: O(n)
- alloc: O(n)
- free: O(1)

---

### 2. Paging

- uso de page tables de 4KB
- criação dinâmica de page tables com pfa_alloc
- uso de temp_map para inicialização

Fluxo:

pfa_alloc → temp_map → memset → instalar PDE

---

### 3. Kernel Heap

#### Estrutura

typedef struct block {
    unsigned int size;
    int free;
    struct block *next;
} block_t;

#### Funcionamento

- heap começa em endereço virtual fixo
- cresce sob demanda (1 página por vez)
- usa first-fit

#### Complexidade

- kmalloc: O(n)
- kfree: O(1)

---

### 4. Integração

PFA → Paging → Heap

---

## Conclusão

O kernel evoluiu de um modelo estático para um sistema dinâmico de gerenciamento de memória, alinhado com práticas reais de sistemas operacionais.
