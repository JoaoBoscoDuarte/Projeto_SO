# 03 — Conceitos Fundamentais

## 1. Modo Protegido x86 (32 bits)

O kernel opera em **modo protegido**, que oferece:
- Endereçamento de até 4 GB de RAM
- Proteção de memória via segmentação e paginação
- Níveis de privilégio (rings 0–3)
- Tratamento de interrupções via IDT

### Rings de Privilégio

| Ring | Nome | Acesso |
|------|------|--------|
| 0 | Kernel mode | Total — I/O, registradores de controle, tudo |
| 3 | User mode | Restrito — sem I/O direto, sem acesso ao kernel |

Este projeto opera inteiramente em **ring 0**.

## 2. Segmentação — GDT

A **Global Descriptor Table** define segmentos de memória. Cada entrada (descriptor) especifica base, limite e flags de acesso.

Entradas usadas neste projeto:

| Índice | Seletor | Descrição |
|--------|---------|-----------|
| 0 | 0x00 | Null descriptor (obrigatório) |
| 1 | 0x08 | Código kernel (ring 0, execute/read) |
| 2 | 0x10 | Dados kernel (ring 0, read/write) |
| 3 | 0x18 | Código usuário (ring 3) |
| 4 | 0x20 | Dados usuário (ring 3) |
| 5 | 0x28 | TSS descriptor |

## 3. Paginação — Two-Level Page Tables

O x86 usa dois níveis de tradução de endereço:

```
Endereço virtual (32 bits):
  [31:22] = índice no Page Directory  (10 bits → 1024 entradas)
  [21:12] = índice na Page Table      (10 bits → 1024 entradas)
  [11:0]  = offset dentro da página   (12 bits → 4096 bytes)
```

### Layout do espaço virtual do kernel

```
0x00000000 – 0xBFFFFFFF  (3 GB) — espaço de usuário
0xC0000000 – 0xC03FFFFF  (4 MB) — kernel (entrada 768 do PD)
0xC0400000 – 0xC07FFFFF  (4 MB) — mapeamentos temporários (entrada 769)
0xC1000000 – ...                 — heap do kernel
0xC2000000 – ...                 — kernel stacks dos processos
```

### Flags de uma PDE/PTE

| Bit | Flag | Significado |
|-----|------|-------------|
| 0 | Present | Página está na memória |
| 1 | R/W | Leitura e escrita permitidas |
| 2 | User | Acessível em ring 3 |
| 7 | PSE | Página de 4MB (só em PDE) |

## 4. Interrupções

### Fluxo de uma interrupção de hardware

```
Hardware gera sinal
       │
       ▼
PIC 8259A recebe o sinal (IRQ n)
       │  traduz para vetor = n + 32 (após remap)
       ▼
CPU consulta IDT[vetor]
       │  salva EIP, CS, EFLAGS na stack
       │  se ring 3→0: também salva ESP, SS e troca para kernel stack (via TSS)
       ▼
Handler assembly (interrupts.s)
       │  pushad + push ds/es
       │  seta DS/ES para seletor do kernel (0x10)
       ▼
Handler C (keyboard_handler_c / pit_handler_c)
       │
       ▼
EOI enviado ao PIC (out 0x20, 0x20)
       │
       ▼
pop es/ds + popad + iretd
       │  restaura EIP, CS, EFLAGS
       │  se havia troca de ring: restaura ESP, SS
       ▼
Execução continua onde parou
```

### Vetores usados

| Vetor | IRQ | Fonte | Handler |
|-------|-----|-------|---------|
| 32 | IRQ0 | Timer PIT | `pit_handler_c` |
| 33 | IRQ1 | Teclado PS/2 | `keyboard_handler_c` |

### Por que remapear o PIC?

Por padrão, o PIC mapeia IRQ0–IRQ7 para vetores 8–15, que conflitam com exceções da CPU (divisão por zero = vetor 0, page fault = vetor 14, etc.). O `pic_remap()` move IRQ0 para vetor 32, evitando conflitos.

## 5. Task State Segment (TSS)

O TSS é necessário para interrupções em ring 3. Quando uma interrupção ocorre em user mode, a CPU precisa saber para qual stack do kernel trocar. Ela lê `ss0` e `esp0` do TSS.

Neste projeto o TSS é usado apenas para manter `esp0` atualizado a cada troca de processo (via `tss_set_kernel_stack()`).

## 6. Convenção de Chamada cdecl

Usada em todas as chamadas C/assembly:

```
Chamador:
  push arg_n        ; argumentos da direita para esquerda
  push arg_1
  call func
  add esp, n*4      ; limpa argumentos

Chamado:
  push ebp          ; salva frame pointer
  mov ebp, esp
  ...
  pop ebp
  ret               ; retorno em EAX
```

Registradores **callee-saved** (o chamado deve preservar): `ebp`, `ebx`, `esi`, `edi`
Registradores **caller-saved** (pode ser destruído): `eax`, `ecx`, `edx`

## 7. I/O em x86

O x86 tem um espaço de endereçamento separado para dispositivos de hardware, acessado com instruções especiais:

```nasm
out 0x20, al    ; escreve AL na porta 0x20 (EOI do PIC)
in  al, 0x60    ; lê da porta 0x60 (scancode do teclado)
```

Em C, usamos wrappers em assembly (`io.s`):

```c
void outb(unsigned short port, unsigned char data);
unsigned char inb(unsigned short port);
```
