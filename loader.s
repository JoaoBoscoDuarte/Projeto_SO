    ; ============================================================================
    ; LOADER.S - Bootloader do Sistema Operacional
    ; ============================================================================
    ; Este arquivo implementa o ponto de entrada do kernel seguindo a
    ; especificação Multiboot, permitindo que o GRUB carregue o SO.
    ; ============================================================================

    global loader                   ; Exporta o símbolo 'loader' como ponto de entrada do ELF

    ; ----------------------------------------------------------------------------
    ; Constantes do Multiboot
    ; ----------------------------------------------------------------------------
    ; O Multiboot é um padrão que permite que bootloaders como GRUB carreguem
    ; kernels de forma padronizada. Requer um cabeçalho específico no início.
    
    MAGIC_NUMBER equ 0x1BADB002     ; Número mágico do Multiboot (identifica o kernel)
    FLAGS        equ 0x0            ; Flags do Multiboot (0x0 = sem requisitos especiais)
    CHECKSUM     equ -MAGIC_NUMBER  ; Checksum: deve fazer MAGIC + FLAGS + CHECKSUM = 0
    KERNEL_STACK_SIZE equ 4096      ; Tamanho da pilha do kernel: 4KB

    ; ----------------------------------------------------------------------------
    ; Seção BSS - Block Started by Symbol (dados não inicializados)
    ; ----------------------------------------------------------------------------
    section .bss
    align 4                         ; Alinha em 4 bytes (requisito de arquitetura x86)
    kernel_stack:                   ; Label que marca o início da pilha
        resb KERNEL_STACK_SIZE      ; Reserva 4096 bytes para a pilha do kernel

    ; ----------------------------------------------------------------------------
    ; Seção TEXT - Código executável
    ; ----------------------------------------------------------------------------
    section .text:                  ; Início da seção de código
    align 4                         ; Código deve estar alinhado em 4 bytes
        dd MAGIC_NUMBER             ; Escreve o número mágico no binário
        dd FLAGS                    ; Escreve as flags
        dd CHECKSUM                 ; Escreve o checksum
                                    ; Estes 3 valores formam o cabeçalho Multiboot

    ; ----------------------------------------------------------------------------
    ; Função loader - Ponto de entrada do kernel
    ; ----------------------------------------------------------------------------
    ; Esta é a primeira função executada quando o GRUB transfere controle
    ; para o kernel. Ela configura o ambiente básico e entra em loop.
    
    loader:                                         ; Label do loader (definido como entry point no linker)
        mov eax, 0xCAFEBABE                         ; Coloca valor 0xCAFEBABE em EAX (para debug/teste)
        mov esp, kernel_stack + KERNEL_STACK_SIZE   ; Configura ESP para apontar para o topo da pilha
                                                    ; (pilha cresce para baixo, então ESP aponta para o fim)
    .loop:
        jmp .loop                   ; Loop infinito - mantém o kernel em execução