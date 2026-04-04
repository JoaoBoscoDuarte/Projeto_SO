    [BITS 32]
    ; =========================================================================
    ; Verifica que está executando em ring 3 lendo o seletor CS:
    ;   CS = 0x1B (0001 1011b) → índice=3, TI=0 (GDT), RPL=3 → ring 3
    ;   CS = 0x08 (0000 1000b) → índice=1, TI=0 (GDT), RPL=0 → ring 0
    ;
    ; Para verificar no Bochs/QEMU:
    ;   - Para a execução e inspeciona os registradores
    ;   - EAX = 0xDEADBEEF confirma que o código executou
    ;   - ECX = 0x0000001B confirma ring 3 (CS com RPL=3)
    ;   - CS  = 0x001B      confirma o seletor de código user
    ; =========================================================================

    mov eax, 0xDEADBEEF     ; marcador visível no log do emulador

    ; Lê o seletor CS atual e salva em ECX para inspeção
    ; Se estiver em ring 3: ECX = 0x1B
    ; Se estiver em ring 0: ECX = 0x08
    mov ecx, cs

    ; Loop infinito — o programa não deve retornar
    ; (não há nada para retornar: não há libc, não há syscall de exit)
.loop:
    jmp .loop
