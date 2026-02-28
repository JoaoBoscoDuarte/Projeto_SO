; ============================================================================
; LOADER.S - Entrada do Kernel (Multiboot + GRUB)
; ============================================================================

; ----------------------------------------------------------------------------
; Cabeçalho Multiboot (deve estar nos primeiros 8KB do binário)
; ----------------------------------------------------------------------------
section .multiboot
align 4
    dd 0x1BADB002          ; Magic number
    dd 0                   ; Flags (0 = padrão)
    dd -(0x1BADB002)       ; Checksum (magic + flags + checksum = 0)

; ----------------------------------------------------------------------------
; Seção BSS - pilha do kernel
; ----------------------------------------------------------------------------
section .bss
align 16
kernel_stack:
    resb 4096              ; 4KB de stack

; ----------------------------------------------------------------------------
; Código do kernel
; ----------------------------------------------------------------------------
section .text
global loader
extern kmain

loader:
    ; Configura stack (pilha cresce para baixo)
    mov esp, kernel_stack + 4096

    ; Chama função principal do kernel
    call kmain

.hang:
    jmp .hang              ; Loop infinito se kmain retornar