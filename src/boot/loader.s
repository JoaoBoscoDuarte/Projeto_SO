    ; ============================================================================
    ; LOADER.S - Bootloader do Sistema Operacional
    ; ============================================================================
    ; Este é o PRIMEIRO código executado quando o GRUB carrega o kernel.
    ; Implementa a especificação Multiboot para ser reconhecido pelo GRUB.
    ; ============================================================================

    global loader                   ; Torna 'loader' visível para o linker (ponto de entrada)

    ; ----------------------------------------------------------------------------
    ; Constantes do Multiboot (Especificação para bootloaders)
    ; ----------------------------------------------------------------------------
    MAGIC_NUMBER equ 0x1BADB002     
    
    ; FLAGS: Bit 0 (page align) + Bit 1 (memory info)
    ; Bit 0 = MULTIBOOT_PAGE_ALIGN: kernel e módulos alinhados em 4KB
    FLAGS        equ 0x00000003     
    
    CHECKSUM     equ -(MAGIC_NUMBER + FLAGS)  
    KERNEL_STACK_SIZE equ 4096

    ; ----------------------------------------------------------------------------
    ; Seção BSS - Block Started by Symbol (dados não inicializados)
    ; ----------------------------------------------------------------------------
    ; Esta seção não ocupa espaço no binário, apenas reserva memória em runtime
    section .bss
    align 4                         ; Alinha em 4 bytes (requisito x86 para performance)
    kernel_stack:                   ; Label que marca o INÍCIO da pilha
        resb KERNEL_STACK_SIZE      ; Reserva 4096 bytes de memória não inicializada

    ; ----------------------------------------------------------------------------
    ; Seção TEXT - Código executável
    ; ----------------------------------------------------------------------------
    section .multiboot              ; Início da seção de código
    align 4                         ; Código deve estar alinhado em 4 bytes
        dd MAGIC_NUMBER             ; Escreve o número mágico no binário
        dd FLAGS                    ; Escreve as flags
        dd -(MAGIC_NUMBER + FLAGS) ; Cálculo explícito para não ter erro de sinal
                                    ; Estes 3 dwords (12 bytes) formam o cabeçalho Multiboot

    extern kmain                    ; Declara que kmain() está definida em outro arquivo (C)
    
    ; ----------------------------------------------------------------------------
    ; Função loader - Ponto de entrada do kernel
    ; ----------------------------------------------------------------------------
    ; O GRUB salta para cá após carregar o kernel na memória.
    ; Neste ponto:
    ; - Estamos em modo protegido 32-bit
    ; - Paginação está desabilitada
    ; - Interrupções estão desabilitadas
    ; - Não há pilha configurada ainda
    loader:
        ; Configura a pilha do kernel
        ; A pilha cresce PARA BAIXO na memória, então ESP deve apontar para o TOPO
        mov esp, kernel_stack + KERNEL_STACK_SIZE   ; ESP = endereço final da pilha
        
        ; Chama a função principal do kernel escrita em C
        call kmain                  ; Transfere controle para kmain()

        ; Se kmain() retornar (não deveria), entra em loop infinito
    .loop:
        jmp .loop                   ; Loop infinito para evitar execução de lixo
