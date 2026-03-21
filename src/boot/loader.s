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

    
    ; ----------------------------------------------------------------------------
    ; Seção DATA - Page directory alinhado em 4096 bytes
    ; ----------------------------------------------------------------------------
    section .data
    align 4096
    page_directory:
        ; 1024 entradas, cada uma mapeando um frame de 4 MB
        ; Bits: PS=1 (4MB page), RW=1, P=1 → 0x83
        ; Entrada i aponta para endereço físico i * 0x400000
        %assign i 0
        %rep 1024
            dd (i * 0x400000) | 0x83   ; presente + leitura/escrita + 4MB page
            %assign i i+1
        %endrep

    ; Declara os módulos externos
    extern kmain                  
    extern kernel_virtual_start
    extern kernel_virtual_end
    extern kernel_physical_start
    extern kernel_physical_end

    section .text
    loader:
        ; salva ebx (endereço do multiboot) antes de qualquer uso de ebx
        mov edi, ebx

        ; --- Ativa identity paging (4 MB pages) ---
        mov eax, page_directory
        mov cr3, eax

        mov eax, cr4
        or  eax, 0x00000010    ; PSE bit
        mov cr4, eax

        mov eax, cr0
        or  eax, 0x80000000    ; PG bit
        mov cr0, eax
        ; A partir daqui: paginação ativa, virtual == físico

        ; configura pilha
        mov esp, kernel_stack + KERNEL_STACK_SIZE

        ; empurra argumentos para kmain
        push kernel_physical_end
        push kernel_physical_start
        push kernel_virtual_end
        push kernel_virtual_start
        push edi                    ; multiboot_addr

        call kmain
        add esp, 20

    .loop:
        jmp .loop
