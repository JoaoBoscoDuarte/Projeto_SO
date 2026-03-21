# Paging e Page Frame Allocator

Este documento descreve a implementação do identity paging e do page frame allocator (PFA) no kernel.

---

## 1. Identity Paging

### O que é

Identity paging é a forma mais simples de paginação: cada endereço virtual é mapeado para o mesmo endereço físico. É o ponto de partida recomendado antes de implementar configurações mais avançadas como o higher-half kernel.

### Por que é necessário

Antes de ativar paginação, o processador acessa memória diretamente pelo endereço físico. Após ativar, todo acesso passa pelo MMU que traduz virtual → físico. Se não houver um mapeamento válido, o CPU gera um page fault imediatamente.

Com identity paging, virtual == físico, então o código continua funcionando normalmente após ligar o paging.

### Implementação

O page directory é criado em compile-time na seção `.data` do `loader.s`, com 1024 entradas de 4 MB cada:

```nasm
section .data
align 4096              ; obrigatório: cr3 usa os 20 bits altos do endereço
page_directory:
    %assign i 0
    %rep 1024
        dd (i * 0x400000) | 0x83   ; P=1 (presente) + RW=1 + PS=1 (4 MB page)
        %assign i i+1
    %endrep
```

O `align 4096` é obrigatório porque os 12 bits baixos de `cr3` são usados para flags, não para endereço. Se não estiver alinhado, o MMU lê o page directory do lugar errado.

O valor `0x83` nas entradas combina três flags:
- `bit 0 (P)` — página presente
- `bit 1 (RW)` — leitura e escrita permitidas
- `bit 7 (PS)` — page size = 4 MB (requer PSE ativo no `cr4`)

### Ativação no loader

```nasm
loader:
    mov eax, page_directory
    mov cr3, eax          ; cr3 = endereço físico do page directory

    mov eax, cr4
    or  eax, 0x00000010   ; bit 4 = PSE: habilita páginas de 4 MB
    mov cr4, eax

    mov eax, cr0
    or  eax, 0x80000000   ; bit 31 = PG: liga o paging
    mov cr0, eax
    ; a partir daqui todo acesso à memória passa pelo MMU
```

A ordem importa: `cr3` deve ser carregado antes de ligar o PG, e o PSE deve ser ativado antes do PG para que as entradas de 4 MB sejam reconhecidas.

### Confirmação no Bochs

Após a implementação, o log do Bochs confirmou:

```
CR0 = PG (bit 31 setado) ✓
CR3 = 0x00103000         ✓  endereço físico do page directory
CR4 = PSE                ✓  páginas de 4 MB ativas
CR2 = 0x00000000         ✓  nenhum page fault ocorreu
```

---

## 2. Page Frame Allocator

### O que é

O page frame allocator (PFA) é o componente responsável por rastrear quais regiões da memória física estão livres ou ocupadas, e fornecer a interface `pfa_alloc()` / `pfa_free()` para o resto do kernel.

### Por que é necessário

Sem um alocador, o kernel não tem como saber quais endereços físicos pode usar com segurança. Usar uma região ocupada pelo BIOS, pelo próprio kernel ou por um módulo do GRUB causaria corrupção de memória.

### Estrutura de dados: bitmap

Cada bit do bitmap representa um page frame de 4 KB. Bit `0` = livre, bit `1` = ocupado.

Com 32 MB de RAM (configuração do Bochs):
- `32 MB / 4 KB = 8192 frames`
- `8192 bits = 256 inteiros de 32 bits = 1 KB de bitmap`

```c
#define PAGE_SIZE    4096
#define MAX_FRAMES   8192
#define BITMAP_SIZE  (MAX_FRAMES / 32)

static unsigned int bitmap[BITMAP_SIZE];
```

### Exportando os limites do kernel via linker

Para o PFA saber onde o kernel está na memória, o `link.ld` exporta labels de início e fim:

```ld
kernel_virtual_start = .;
kernel_physical_start = .;

.text  ALIGN(0x1000) : { *(.text*)  }
.rodata ALIGN(0x1000) : { *(.rodata*) }
.data  ALIGN(0x1000) : { *(.data*)  }
.bss   ALIGN(0x1000) : { *(COMMON) *(.bss*) }

kernel_virtual_end = .;
kernel_physical_end = .;
```

Esses labels são lidos no `loader.s` e passados como argumentos para `kmain`:

```nasm
push kernel_physical_end
push kernel_physical_start
push kernel_virtual_end
push kernel_virtual_start
push edi                    ; multiboot_addr (ebx salvo antes de qualquer uso)
call kmain
add esp, 20                 ; limpa 5 argumentos (5 * 4 bytes)
```

A assinatura de `kmain` foi atualizada para receber esses valores:

```c
void kmain(unsigned int multiboot_addr,
           unsigned int kernel_virtual_start,
           unsigned int kernel_virtual_end,
           unsigned int kernel_physical_start,
           unsigned int kernel_physical_end)
```

### Inicialização do PFA

A estratégia é conservadora: começa marcando tudo como ocupado e só libera o que o GRUB confirma como disponível. Depois re-marca as regiões que nunca podem ser usadas:

```c
void pfa_init(unsigned int kphys_start, unsigned int kphys_end,
              multiboot_info_t *mbinfo)
{
    // 1. tudo ocupado por padrão
    for (unsigned int i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFF;

    // 2. libera regiões que o GRUB marcou como disponíveis (via mmap)
    if (mbinfo->flags & 0x40) {
        // percorre o mapa de memória do GRUB
        // para cada região MULTIBOOT_MEMORY_AVAILABLE: set_free(frame)
    }

    // 3. re-marca regiões que não podem ser usadas
    mark_range_used(0x00000000, 0x00100000); // 1 MB baixo: BIOS, GRUB, I/O
    mark_range_used(kphys_start, kphys_end); // o próprio kernel
    // módulos carregados pelo GRUB também são marcados
}
```

O passo 2 é necessário porque o GRUB não marca automaticamente a região do kernel como reservada — ele reporta toda a memória acima de 1 MB como disponível. Por isso o passo 3 existe: corrige essa omissão.

### Interface pública

```c
unsigned int pfa_alloc(void);      // retorna endereço físico de um frame livre
void         pfa_free(unsigned int addr);  // devolve um frame ao pool
```

### Confirmação do funcionamento

```
kernel virtual:  0x100000 - 0x107C40
kernel physical: 0x100000 - 0x107C40
Tamanho do kernel: 31KB

frame1: 0x109000   ← primeiro frame livre acima do kernel
frame2: 0x10A000   ← frame1 + 4 KB ✓
frame3: 0x10B000   ← frame2 + 4 KB ✓
frame4: 0x109000   ← igual ao frame1 após pfa_free(frame1) ✓
```

Nenhum frame abaixo de `0x100000` foi retornado — o 1 MB baixo está protegido. Nenhum frame dentro da região do kernel foi retornado — o `mark_range_used` funcionou corretamente.

---

## Próximos passos

### Higher-half kernel

Mover o kernel para o endereço virtual `0xC0100000` (3 GB + 1 MB), mantendo-o carregado fisicamente em `0x00100000`. Isso libera o espaço virtual abaixo de 3 GB para processos de usuário.

Requer mudanças coordenadas em três arquivos:

- **`link.ld`** — usar `AT()` para separar endereço virtual do físico:
  ```ld
  . = 0xC0100000;
  .text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000) { ... }
  ```

- **`loader.s`** — page directory com dois mapeamentos antes de ligar o paging:
  - entrada 0 → `0x00000000` (identity temporário dos primeiros 4 MB)
  - entrada 768 → `0x00000000` (mapeia `0xC0000000` virtual → `0x00000000` físico)
  - após ligar paging: salto absoluto para label no higher-half
  - após o salto: remove entrada 0 e invalida TLB com `invlpg [0]`

- **`kmain.c` e `pfa.c`** — endereços físicos recebidos do GRUB precisam de `+ 0xC0000000` para se tornarem endereços virtuais válidos após o identity mapping ser removido.

### Seção 10.2 — Acesso a page frames alocados

Com páginas de 4 KB no higher-half, `pfa_alloc()` retornará endereços físicos não mapeados. Será necessário um mecanismo de mapeamento temporário (entrada 1023 da page table do kernel, endereço `0xC03FF000`) para poder escrever nesses frames antes de adicioná-los ao page directory.

### Kernel Heap

Com o PFA funcionando, implementar `malloc` e `free` para o kernel substituindo `sbrk/brk` por chamadas ao `pfa_alloc`.
