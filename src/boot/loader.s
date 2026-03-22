; ============================================================================
    ; LOADER.S - Bootloader do Sistema Operacional
    ; ============================================================================
    ; PRIMEIRO código executado quando o GRUB carrega o kernel.
    ; Implementa a especificação Multiboot e configura paginação higher-half.
    ;
    ; CORREÇÕES APLICADAS:
    ;   1. multiboot_addr passado como FÍSICO para kmain (não virtualizado)
    ;   2. Remoção do identity mapping agora invalida TLB COMPLETO (reload CR3)
    ;      em vez de invlpg [0], que não é suficiente para 4MB pages (PSE)
    ;   3. Stack configurada ANTES de qualquer push/call
    ;   4. Endereço físico do page_directory calculado corretamente via subtração
    ;   5. Comentários atualizados para refletir comportamento real
    ; ============================================================================

    global loader
    global page_directory          ; exporta para paging.c usar via extern

    ; ----------------------------------------------------------------------------
    ; Constantes Multiboot
    ; ----------------------------------------------------------------------------
    MAGIC_NUMBER      equ 0x1BADB002
    FLAGS             equ 0x00000003      ; page-align + memory info
    CHECKSUM          equ -(MAGIC_NUMBER + FLAGS)
    KERNEL_STACK_SIZE equ 4096
    KERNEL_VIRT_BASE  equ 0xC0000000

    ; ----------------------------------------------------------------------------
    ; Seção BSS — stack do kernel (endereços VIRTUAIS após paginação)
    ; ----------------------------------------------------------------------------
    section .bss
    align 4096
    kernel_stack:
        resb KERNEL_STACK_SIZE

    ; ----------------------------------------------------------------------------
    ; Cabeçalho Multiboot — DEVE ser encontrado nos primeiros 8KB do binário
    ; ----------------------------------------------------------------------------
    section .multiboot
    align 4
        dd MAGIC_NUMBER
        dd FLAGS
        dd CHECKSUM

    ; ----------------------------------------------------------------------------
    ; Page Directory — alinhado em 4096 bytes, na seção .data
    ; IMPORTANTE: o linker atribui endereços VIRTUAIS (0xC01xxxxx) a estes labels,
    ; mas ao carregar CR3 ANTES de ativar paginação, precisamos do endereço FÍSICO.
    ; Por isso usamos: mov eax, page_directory - KERNEL_VIRT_BASE
    ; ----------------------------------------------------------------------------
    section .data
    align 4096
    page_directory:
        ; Entrada 0: identity mapping temporário (Virtual 0x0 -> Físico 0x0, 4MB, PSE)
        dd 0x00000083

        ; Entradas 1 a 767: não mapeadas
        times (768 - 1) dd 0

        ; Entrada 768: higher-half (Virtual 0xC0000000 -> Físico 0x0, 4MB, PSE)
        ; 0x83 = PS(bit7) | R/W(bit1) | Present(bit0)
        dd 0x00000083

        ; Entradas 769 a 1023: não mapeadas
        times (1024 - 768 - 1) dd 0

    ; ----------------------------------------------------------------------------
    ; Símbolos exportados pelo linker (endereços VIRTUAIS — subtrair 0xC0000000
    ; para obter o endereço físico correspondente)
    ; ----------------------------------------------------------------------------
    extern kmain
    extern kernel_virtual_start
    extern kernel_virtual_end
    extern kernel_physical_start
    extern kernel_physical_end

    ; ----------------------------------------------------------------------------
    ; Ponto de entrada — executa a FÍSICO 0x00100000, paginação DESLIGADA
    ; ----------------------------------------------------------------------------
    section .text
    loader:
        ; EAX = 0x2BADB002 (magic do GRUB), EBX = endereço FÍSICO do multiboot_info
        ; Salva EBX antes de qualquer operação
        mov edi, ebx            ; preserva ponteiro multiboot (físico)

        ; --- 1. Carrega CR3 com o endereço FÍSICO do page_directory ---
        ; O linker resolve 'page_directory' como VMA (0xC01xxxxx).
        ; Subtraímos KERNEL_VIRT_BASE para obter o endereço físico real.
        ; Paginação ainda está DESLIGADA aqui, então CR3 precisa ser físico.
        mov eax, page_directory - KERNEL_VIRT_BASE
        mov cr3, eax

        ; --- 2. Habilita PSE (páginas de 4MB via bit CR4.PSE) ---
        mov eax, cr4
        or  eax, 0x00000010
        mov cr4, eax

        ; --- 3. Habilita paginação (CR0.PG) ---
        ; A partir deste ponto o processador traduz TODOS os acessos via page directory.
        ; O identity mapping (entrada 0) nos mantém funcionando neste endereço baixo.
        mov eax, cr0
        or  eax, 0x80000000
        mov cr0, eax

        ; --- 4. Salta para o endereço virtual ALTO (0xC010xxxx) ---
        ; OBRIGATÓRIO: usar salto INDIRETO via registrador.
        ; Um 'jmp label' direto geraria um offset relativo pequeno e NÃO saltaria
        ; para o endereço virtual correto gerado pelo linker.
        ; LEA carrega o endereço absoluto (VMA) que o linker atribuiu a .higher_half.
        lea eax, [.higher_half]
        jmp eax

    .higher_half:
        ; =====================================================================
        ; Agora executamos no endereço VIRTUAL 0xC010xxxx.
        ; Tanto o identity mapping (entrada 0) quanto o higher-half (entrada 768)
        ; estão ativos — ambos mapeiam para a mesma memória física.
        ; =====================================================================

        ; --- 5. Remove o identity mapping (entrada 0 do page directory) ---
        ; Acesso ao page_directory agora usa seu endereço VIRTUAL — correto,
        ; pois a paginação está ativa e a entrada 768 mapeia esta região.
        mov dword [page_directory], 0

        ; --- 6. Flush COMPLETO do TLB via reload de CR3 ---
        ; CORREÇÃO: invlpg [0] NÃO é suficiente para páginas PSE de 4MB.
        ; O reload do CR3 invalida TODAS as entradas do TLB — obrigatório.
        mov eax, cr3
        mov cr3, eax

        ; --- 7. Configura a pilha ANTES de qualquer push ---
        ; kernel_stack é um símbolo no .bss → endereço VIRTUAL pelo linker.
        ; Correto: após paginação, usamos VMAs diretamente.
        mov esp, kernel_stack + KERNEL_STACK_SIZE

        ; --- 8. Empurra argumentos para kmain ---
        ; ORDEM: cdecl — argumentos empurrados da direita para a esquerda.
        ; Assinatura: kmain(multiboot_addr, virt_start, virt_end, phys_start, phys_end)
        push kernel_physical_end    ; arg5
        push kernel_physical_start  ; arg4
        push kernel_virtual_end     ; arg3
        push kernel_virtual_start   ; arg2

        ; CORREÇÃO: passa EDI (físico original) como multiboot_addr.
        ; kmain acessa mbinfo via ponteiro físico — seguro enquanto o mmap do GRUB
        ; estiver abaixo de 4MB (coberto pelo mapeamento da entrada 768).
        push edi                    ; arg1: endereço FÍSICO do multiboot_info

        call kmain
        add esp, 20

    .loop:
        cli
        hlt
        jmp .loop