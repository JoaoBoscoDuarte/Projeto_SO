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
    MAGIC_NUMBER equ 0x1BADB002     
    
    ; FLAGS: Bit 0 (alinhamento) + Bit 1 (info de memória)
    ; Isso garante que o GRUB configure a máquina de forma mais estável.
    FLAGS        equ 0x00000003     
    
    CHECKSUM     equ -(MAGIC_NUMBER + FLAGS)  
    KERNEL_STACK_SIZE equ 4096

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
    section .multiboot                   ; Início da seção de código
    align 4                         ; Código deve estar alinhado em 4 bytes
        dd MAGIC_NUMBER             ; Escreve o número mágico no binário
        dd FLAGS                    ; Escreve as flags
        dd -(MAGIC_NUMBER + FLAGS) ; Cálculo explícito para não ter erro de sinal
                                    ; Estes 3 valores formam o cabeçalho Multiboot

    ; ----------------------------------------------------------------------------
    ; Função loader - Ponto de entrada do kernel
    ; ----------------------------------------------------------------------------
    ; Esta é a primeira função executada quando o GRUB transfere controle
    ; para o kernel. Ela configura o ambiente básico e entra em loop.

    extern kmain
    
    loader:                                         ; Label do loader (definido como entry point no linker)
        mov esp, kernel_stack + KERNEL_STACK_SIZE   ; Configura ESP para apontar para o topo da pilha
                                                    ; (pilha cresce para baixo, então ESP aponta para o fim)
        
        call kmain

    .loop:
        jmp .loop                   ; Loop infinito - mantém o kernel em execução