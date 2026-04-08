# 02 — Processo de Boot

## Visão Geral

```
Liga o PC
   │
   ▼
BIOS (POST + busca dispositivo bootável)
   │
   ▼
GRUB (lê grub.cfg, carrega kernel.elf em 0x00100000)
   │
   ▼
loader.s (executa em 0x00100000, paginação DESLIGADA)
   │  1. Carrega CR3 com endereço físico do page_directory
   │  2. Habilita PSE (páginas 4MB) em CR4
   │  3. Liga paginação em CR0
   │  4. Salta para endereço virtual alto (0xC010xxxx)
   │  5. Remove identity map (entrada 0 do PD)
   │  6. Flush TLB (reload CR3)
   │  7. Configura ESP para kernel_stack
   │  8. Empurra argumentos e chama kmain()
   │
   ▼
kmain() — inicializa subsistemas em ordem obrigatória
   │
   ▼
shell_run() — loop infinito aguardando comandos
```

## 1. GRUB e Multiboot

O GRUB carrega o kernel baseado no cabeçalho Multiboot presente nos primeiros 8 KB do binário:

```nasm
section .multiboot
align 4
    dd 0x1BADB002          ; magic number
    dd 0x00000003          ; flags: page-align + memory info
    dd -(0x1BADB002 + 3)   ; checksum: magic + flags + checksum = 0
```

Ao transferir controle, o GRUB garante:
- `EAX = 0x2BADB002` (confirma boot Multiboot)
- `EBX` = endereço **físico** da struct `multiboot_info_t`
- CPU em modo protegido 32 bits, paginação **desligada**

## 2. loader.s — Paginação e Salto Higher-Half

O kernel é linkado com VMA `0xC0100000` mas carregado fisicamente em `0x00100000`. O `loader.s` precisa ativar paginação antes de qualquer acesso a endereços virtuais altos.

### Page Directory inicial (em `.data`, alinhado em 4096)

```nasm
page_directory:
    dd 0x00000083    ; entrada 0:   identity map 0x0→0x0 (4MB, PSE) — temporário
    times 767 dd 0   ; entradas 1–767: não mapeadas
    dd 0x00000083    ; entrada 768: higher-half 0xC0000000→0x0 (4MB, PSE)
    times 255 dd 0   ; entradas 769–1023: não mapeadas
```

`0x83 = Present | R/W | PageSize(4MB)`

### Sequência de ativação

```nasm
; 1. CR3 = endereço FÍSICO do page_directory
mov eax, page_directory - 0xC0000000
mov cr3, eax

; 2. Habilita PSE (páginas 4MB)
mov eax, cr4
or  eax, 0x10
mov cr4, eax

; 3. Liga paginação
mov eax, cr0
or  eax, 0x80000000
mov cr0, eax

; 4. Salto indireto para endereço virtual alto
lea eax, [.higher_half]
jmp eax
```

O salto **indireto** via registrador é obrigatório — um `jmp label` direto geraria offset relativo pequeno e não saltaria para `0xC010xxxx`.

### Após o salto

```nasm
.higher_half:
    ; Remove identity map (entrada 0)
    mov dword [page_directory], 0
    ; Flush TLB completo
    mov eax, cr3
    mov cr3, eax
    ; Configura stack
    mov esp, kernel_stack + 4096
    ; Empurra argumentos para kmain (cdecl, direita para esquerda)
    push kernel_physical_end
    push kernel_physical_start
    push kernel_virtual_end
    push kernel_virtual_start
    push edi                    ; multiboot_addr (físico)
    call kmain
```

## 3. kmain() — Ordem de Inicialização

A ordem é obrigatória — cada subsistema depende do anterior:

```c
fb_clear();                                    // 1. Limpa tela (sem dependências)
pfa_init(kphys_start, kphys_end, mbinfo);      // 2. Bitmap de frames físicos
paging_init();                                 // 3. Troca PSE 4MB por page tables 4KB
gdt_init();                                    // 4. Segmentos de memória
tss_init(0x10, kernel_virtual_end);            // 5. Task State Segment
serial_init();                                 // 6. Porta serial (debug)
idt_init();                                    // 7. Tabela de interrupções
pic_remap();                                   // 8. Remapeia PIC (IRQ0=32, IRQ1=33)
pit_init(100);                                 // 9. Timer 100 Hz
kheap_init();                                  // 10. Heap do kernel
process_init();                                // 11. Tabela de processos (PID 0 = kernel)
asm volatile("sti");                           // 12. Habilita interrupções
shell_run();                                   // 13. Loop principal (nunca retorna)
```

### Por que pfa_init antes de paging_init?

`paging_init()` pode chamar `pfa_alloc()` internamente para alocar page tables. Se o PFA não estiver inicializado, `pfa_alloc()` retorna 0 e a paginação falha.

### Por que gdt_init antes de tss_init?

`tss_init()` instala o descritor TSS na entrada 5 da GDT. A GDT precisa existir antes.

### Por que sti por último?

Com interrupções habilitadas, o PIT começa a disparar IRQ0 e o teclado pode gerar IRQ1. Todos os handlers precisam estar registrados na IDT antes disso.
