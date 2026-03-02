; ============================================================================
; IO.S - Funções de I/O de portas
; ============================================================================
; x86 possui portas de I/O (0x0000-0xFFFF) para comunicar com hardware.
; As instruções IN e OUT só funcionam em assembly, então criamos wrappers.
; ============================================================================

global outb     ; Exporta outb para ser usada em C
global inb      ; Exporta inb para ser usada em C

; ============================================================================
; outb - Envia um byte para uma porta I/O
; ============================================================================
; Convenção de chamada C (cdecl):
; Parâmetros são empilhados da direita para esquerda
; Stack layout:
;   [esp + 8] = data (byte a enviar)
;   [esp + 4] = port (número da porta)
;   [esp    ] = endereço de retorno
outb:
    mov al, [esp + 8]   ; AL = data (registrador de 8 bits)
    mov dx, [esp + 4]   ; DX = port (portas usam registrador DX)
    out dx, al          ; Envia AL para a porta DX
    ret                 ; Retorna para o chamador

; ============================================================================
; inb - Lê um byte de uma porta I/O
; ============================================================================
; Stack layout:
;   [esp + 4] = port (número da porta)
;   [esp    ] = endereço de retorno
; Retorno: valor lido fica em AL (convenção C)
inb:
    mov dx, [esp + 4]   ; DX = port
    in al, dx           ; Lê da porta DX e armazena em AL
    ret                 ; AL contém o valor de retorno
