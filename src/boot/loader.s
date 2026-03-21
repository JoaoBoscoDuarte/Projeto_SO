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
        ; Entrada 0: Identity mapping temporário (Virtual 0x0 -> Físico 0x0)
        dd 0x00000083
        
        ; Entradas 1 a 767: Vazias
        times (768 - 1) dd 0
        
        ; Entrada 768: Higher-half mapping (Virtual 0xC0000000 -> Físico 0x0)
        ; Isso mapeia a faixa 3GB~3GB+4MB para a física 0~4MB
        dd 0x00000083
        
        ; Entradas 769 a 1023: Vazias
        times (1024 - 768 - 1) dd 0

    extern kmain                  
    extern kernel_virtual_start
    extern kernel_virtual_end
    extern kernel_physical_start
    extern kernel_physical_end

    section .text
    loader:
        ; Transforma o endereço do multiboot em VIRTUAL adicionando 3GB
        mov edi, ebx
        add edi, 0xC0000000

        ; Subtrai 3GB do endereço do page_directory para achar o endereço FÍSICO dele
        mov eax, page_directory - 0xC0000000
        mov cr3, eax

        mov eax, cr4
        or  eax, 0x00000010    ; PSE bit
        mov cr4, eax

        mov eax, cr0
        or  eax, 0x80000000    ; PG bit
        mov cr0, eax

        ; === O GRANDE SALTO ===
        ; Até agora, o EIP (ponteiro de instrução) está no endereço físico baixo (1MB).
        ; Ao saltarmos para uma label absoluta, forçamos o EIP a pular para o endereço 
        ; virtual (3GB) gerado pelo linker.
        lea eax, .higher_half
        jmp eax

    .higher_half:
        ; Agora estamos rodando a 3GB de altura! O mapeamento temporário (identity) 
        ; não é mais necessário. Desmapeamos a página 0 e limpamos o TLB por segurança.
        mov dword [page_directory - 0xC0000000], 0
        invlpg [0]

        ; configura pilha
        mov esp, kernel_stack + KERNEL_STACK_SIZE

        ; empurra argumentos para kmain
        push kernel_physical_end
        push kernel_physical_start
        push kernel_virtual_end
        push kernel_virtual_start
        push edi                    ; multiboot_addr virtualizado

        call kmain
        add esp, 20

    .loop:
        jmp .loop
