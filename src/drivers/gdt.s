; ============================================================================
; GDT.S - Carrega a GDT no processador
; ============================================================================

global gdt_flush    ; Exporta para C

; ============================================================================
; gdt_flush - Carrega a nova GDT e atualiza registradores de segmento
; ============================================================================
; Parâmetro: ponteiro para gdt_ptr (passado na pilha)
gdt_flush:
    mov eax, [esp + 4]  ; EAX = endereço da estrutura gdt_ptr
    lgdt [eax]          ; LGDT: Load Global Descriptor Table
                        ; Carrega a GDT no registrador GDTR do processador
    
    xchg bx, bx         ; MAGIC BREAKPOINT do Bochs
                        ; Esta instrução é NOP normal, mas o Bochs
                        ; a reconhece e pausa a execução para debug
    
    ; Atualiza os registradores de segmento de DADOS
    ; Todos apontam para o segmento 2 da GDT (offset 0x10)
    mov ax, 0x10        ; 0x10 = 16 = segundo segmento (dados)
    mov ds, ax          ; DS = Data Segment
    mov es, ax          ; ES = Extra Segment
    mov fs, ax          ; FS = Extra Segment
    mov gs, ax          ; GS = Extra Segment
    mov ss, ax          ; SS = Stack Segment
    
    ; Atualiza o registrador de segmento de CÓDIGO (CS)
    ; CS não pode ser modificado diretamente, precisa de FAR JUMP
    ; Far jump: salta para um endereço em outro segmento
    jmp 0x08:.flush     ; Salta para segmento 0x08 (código), label .flush
                        ; Isso força o CS a ser atualizado para 0x08
.flush:
    ret                 ; Retorna para C
