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

    ; ----------------------------------------------------------------------------
    ; Função loader - Ponto de entrada do kernel
    ; ----------------------------------------------------------------------------
    ; O GRUB salta para cá após carregar o kernel na memória.
    ; Neste ponto:
    ; - Estamos em modo protegido 32-bit
    ; - Paginação está desabilitada         ← ainda verdade AO ENTRAR em loader
    ; - Interrupções estão desabilitadas
    ; - Não há pilha configurada ainda
    loader:
        
        ; --- Ativa identity paging (4 MB pages) ---
        ; Carrea o endereço físico do page directory em cr3. 
        ; O MMU usa cr3 para saber onde está o PDT. Como ainda não há paging ativo, o endereço de page_directory já é físico.
        mov eax, page_directory
        mov cr3, eax

        ; Lê cr4, seta o bit 4 (PSE — Page Size Extensions) e escreve de volta.
        ; Sem isso, o bit PS nas entradas do PDT é ignorado e o hardware tentaria usar page tables de 4 KB, o que quebraria tudo
        mov eax, cr4
        or  eax, 0x00000010    ; PSE bit
        mov cr4, eax

        ; Lê cr0, seta o bit 31 (PG — Paging Enable) e escreve de volta. Este é o momento exato em que o paging é ligado. 
        ; A partir da próxima instrução, todo acesso à memória passa pelo MMU.
        mov eax, cr0
        or  eax, 0x80000000    ; PG bit
        mov cr0, eax
        ; A partir daqui: paginação ativa, virtual == físico

        ; --- Configura pilha ---
        ; A pilha cresce PARA BAIXO na memória, então ESP deve apontar para o TOPO
        mov esp, kernel_stack + KERNEL_STACK_SIZE   ; ESP = endereço final da pilha
        ; A partir daqui: pilha disponível

        ; Empurrando os módulo externos na pilha antes do call main
        push kernel_physical_end
        push kernel_physical_start
        push kernel_virtual_end
        push kernel_virtual_start
        push ebx                    ; primeiro argumento = multiboot_info


        call kmain                  ; Transfere controle para kmain(unsigned int multiboot_info)
        add esp, 20                 ; Limpa 5 argumentos (5 * 4 bytes)

        ; Se kmain() retornar (não deveria), entra em loop infinito
    .loop:
        jmp .loop                   ; Loop infinito para evitar execução de lixo
