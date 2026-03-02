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
    ; O Multiboot é um padrão que permite que bootloaders como GRUB carreguem
    ; kernels de forma padronizada. Requer um cabeçalho específico no início.
    
    MAGIC_NUMBER equ 0x1BADB002     ; Número mágico que identifica um kernel Multiboot
                                    ; O GRUB procura por este valor no binário
    
    FLAGS        equ 0x0            ; Flags do Multiboot (0x0 = sem requisitos especiais)
    
    CHECKSUM     equ -MAGIC_NUMBER  ; Checksum: MAGIC + FLAGS + CHECKSUM = 0
                                    ; Validação de integridade do cabeçalho
    
    KERNEL_STACK_SIZE equ 4096      ; Tamanho da pilha: 4KB (suficiente para início)

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
    ; DEVE estar nos primeiros 8KB do binário para o GRUB encontrar
    section .text:
    align 4                         ; Alinhamento obrigatório de 4 bytes
        dd MAGIC_NUMBER             ; Escreve o número mágico (4 bytes)
        dd FLAGS                    ; Escreve as flags (4 bytes)
        dd CHECKSUM                 ; Escreve o checksum (4 bytes)
                                    ; Estes 12 bytes formam o cabeçalho Multiboot

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
