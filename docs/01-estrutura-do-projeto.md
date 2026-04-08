# 01 — Estrutura do Projeto

## Árvore de Diretórios

```
projeto_so/
├── src/
│   ├── boot/
│   │   ├── loader.s          # Ponto de entrada: Multiboot, paginação, salto para kmain
│   │   └── usermode.s        # Transição ring 0 → ring 3 via iret
│   ├── drivers/
│   │   ├── fb.c              # Framebuffer VGA modo texto 80x25
│   │   ├── gdt.c / gdt.s     # Global Descriptor Table
│   │   ├── tss.c / tss.s     # Task State Segment
│   │   ├── idt.c             # Interrupt Descriptor Table
│   │   ├── interrupts.s      # Handlers IRQ0 (timer) e IRQ1 (teclado)
│   │   ├── pic.c             # Programmable Interrupt Controller 8259A
│   │   ├── pit.c             # Programmable Interval Timer — 100 Hz
│   │   ├── keyboard.c        # Driver PS/2 com buffer circular
│   │   ├── paging.c          # Subsistema de paginação 4KB
│   │   ├── pfa.c             # Page Frame Allocator (bitmap)
│   │   ├── serial.c          # Porta serial COM1 (debug)
│   │   └── io.s              # outb / inb em assembly
│   ├── kernel/
│   │   ├── kmain.c           # Ponto de entrada C — inicializa subsistemas
│   │   ├── kheap.c           # Heap do kernel (free-list)
│   │   ├── process.c         # PCB, tabela de processos, criação
│   │   ├── scheduler.c       # Scheduler cooperativo round-robin
│   │   ├── switch.s          # context_switch em assembly
│   │   ├── shell.c           # Mini-shell interativo
│   │   └── top.c             # Monitor de processos em tempo real
│   ├── lib/
│   │   ├── printf.c          # kprintf com suporte a %d, %x, %s
│   │   ├── string.c          # strcmp, strlen, memset, memcpy, atoi
│   │   └── cpuid.c           # Leitura de vendor/brand do CPU via CPUID
│   └── include/              # Headers públicos de todos os módulos
├── docs/                     # Esta documentação
├── iso/                      # Estrutura da imagem ISO bootável
├── link.ld                   # Linker script — higher-half kernel
├── Makefile                  # Build, ISO, Docker, execução
└── bochsrc.txt               # Configuração do emulador Bochs
```

## Fluxo de Build

```
loader.s  ──nasm──►  loader.o  ─┐
kmain.c   ──gcc───►  kmain.o   ─┤
drivers/  ──gcc───►  *.o       ─┼──ld──► kernel.elf ──► os.iso ──► bochs
kernel/   ──gcc───►  *.o       ─┤
lib/      ──gcc───►  *.o       ─┘
```

### Comandos principais

| Comando | Ação |
|---------|------|
| `make` | Compila o kernel |
| `make os.iso` | Gera a imagem ISO |
| `make run` | Compila e executa no Bochs |
| `make clean` | Remove artefatos de build |
| `make docker-run` | Compila e executa via Docker + QEMU (macOS/ARM) |

## Flags de Compilação

```makefile
-m32                  # Alvo 32 bits
-nostdlib -nostdinc   # Sem biblioteca padrão
-fno-builtin          # Sem funções built-in do GCC
-fno-stack-protector  # Sem stack canary (requer SO)
-fno-pie -fno-pic     # Sem position-independent code
-Wall -Wextra -Werror # Todos os warnings são erros
```

## Layout de Memória Virtual

```
0x00000000 – 0xBFFFFFFF  espaço de usuário (futuro)
0xC0000000 – 0xC03FFFFF  kernel (4 MB, entrada 768 do PD)
0xC0400000 – 0xC07FFFFF  mapeamentos temporários (entrada 769)
0xC1000000 – ...         heap do kernel (cresce sob demanda)
0xC2000000 – ...         kernel stacks dos processos
                         PID n → 0xC2000000 + n * 0x2000
```
