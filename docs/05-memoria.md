# 05 — Memória

## 1. Paginação Higher-Half

### Motivação

O kernel é linkado com VMA `0xC0100000` mas carregado fisicamente em `0x00100000`. Isso é chamado de **higher-half kernel**: o kernel ocupa o topo do espaço virtual (acima de 3 GB), deixando os 3 GB inferiores para processos de usuário.

### Inicialização em dois estágios

**Estágio 1 — loader.s (PSE 4MB):**

O `loader.s` usa páginas de 4MB (PSE) para ativar paginação rapidamente:
- Entrada 0 do PD: identity map `0x0 → 0x0` (temporário)
- Entrada 768 do PD: higher-half `0xC0000000 → 0x0`

Após o salto para o endereço virtual alto, a entrada 0 é removida.

**Estágio 2 — paging_init() (page tables 4KB):**

`paging_init()` substitui as entradas PSE por page tables reais de 4KB:

```c
// Entrada 768: kernel_pt cobre 0xC0000000–0xC03FFFFF
// Mapeia i*4KB → i*4KB para i = 0..1023
for (i = 0; i < 1024; i++)
    kernel_pt[i] = (i * PAGE_SIZE) | PAGE_KERNEL;

page_directory[768] = VIRT_TO_PHYS(kernel_pt) | PAGE_KERNEL;

// Entrada 769: temp_pt para mapeamentos temporários
page_directory[769] = VIRT_TO_PHYS(temp_pt) | PAGE_KERNEL;
```

### Mapeamento temporário (temp_map)

Para acessar um frame físico recém-alocado (que ainda não tem endereço virtual), usa-se o slot fixo `0xC07FF000`:

```c
void *ptr = temp_map(frame_fisico);  // mapeia em 0xC07FF000
// escreve/lê via ptr
temp_unmap();                         // remove o mapeamento
```

Só existe **um** slot temporário — não chamar `temp_map` duas vezes sem `temp_unmap` no meio.

### API de paginação

| Função | Descrição |
|--------|-----------|
| `paging_init()` | Substitui PSE por page tables 4KB |
| `paging_map(virt, phys, flags)` | Mapeia uma página 4KB |
| `paging_unmap(virt)` | Remove mapeamento |
| `paging_get_phys(virt)` | Retorna endereço físico de um virtual |
| `temp_map(phys)` | Mapeamento temporário em 0xC07FF000 |
| `temp_unmap()` | Remove mapeamento temporário |

### Macros úteis

```c
VIRT_TO_PHYS(vaddr)  // vaddr - 0xC0000000
PHYS_TO_VIRT(paddr)  // paddr + 0xC0000000
PD_INDEX(vaddr)      // bits 31:22
PT_INDEX(vaddr)      // bits 21:12
PAGE_ALIGN_UP(addr)
PAGE_ALIGN_DOWN(addr)
```

---

## 2. Page Frame Allocator (`pfa.c`)

### Estrutura de dados

O PFA usa um **bitmap** onde cada bit representa um frame físico de 4KB:
- `bit = 1` → frame ocupado
- `bit = 0` → frame livre

O bitmap é colocado logo após o kernel na memória física, em endereço calculado em runtime.

### Inicialização

```
1. Descobre o tamanho total da RAM via mmap do GRUB
2. Calcula total_frames = RAM / 4KB
3. Posiciona o bitmap após o kernel + módulos GRUB
4. Marca TUDO como ocupado (bitmap = 0xFFFFFFFF)
5. Libera regiões marcadas como AVAILABLE pelo GRUB
6. Re-marca como ocupado:
   - 0x0 – 0xFFFFF  (1MB baixo: BIOS, GRUB, I/O)
   - kernel físico
   - o próprio bitmap
   - módulos GRUB
```

A estratégia conservadora (tudo ocupado por padrão) evita usar memória que o GRUB não reportou explicitamente como disponível.

### API

| Função | Descrição |
|--------|-----------|
| `pfa_alloc()` | Retorna endereço físico de um frame livre (first-fit) |
| `pfa_free(addr)` | Devolve frame ao pool |
| `pfa_total_frames()` | Total de frames gerenciados |
| `pfa_used_frames()` | Frames atualmente ocupados |
| `pfa_free_frames()` | Frames disponíveis |

---

## 3. Heap do Kernel (`kheap.c`)

### Arquitetura

A heap cresce a partir de `0xC1000000`, expandindo sob demanda via `pfa_alloc()` + `paging_map()`.

Cada bloco tem um header:

```c
typedef struct block {
    unsigned int  size;   // bytes de dados (sem o header)
    int           free;   // 1 = livre, 0 = ocupado
    struct block *next;   // próximo bloco na lista
} block_t;
```

### Estratégia: first-fit com split e coalescing

**Alocação (`kmalloc`):**
1. Percorre a free-list procurando bloco livre com `size >= pedido`
2. Se encontrar e o bloco for grande o suficiente, faz **split** (divide em dois)
3. Se não encontrar, expande a heap com uma nova página

**Liberação (`kfree`):**
1. Marca o bloco como livre
2. **Coalescing**: funde com o bloco seguinte se também estiver livre

### API

| Função | Descrição |
|--------|-----------|
| `kheap_init()` | Aloca primeira página da heap |
| `kmalloc(size)` | Aloca `size` bytes (alinhado a 4 bytes) |
| `kfree(ptr)` | Libera bloco |
| `kheap_used_bytes()` | Bytes ocupados na heap |
| `kheap_total_bytes()` | Bytes totais mapeados |
