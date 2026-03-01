global gdt_flush
gdt_flush:
    mov eax, [esp + 4]  ; Pega o ponteiro da GDT passado como argumento
    lgdt [eax]          ; Carrega a GDT
    
    xchg bx, bx         ; <--- ADICIONE ISSO: O Bochs vai congelar aqui e abrir o terminal

    ; Atualiza os registros de dados com o offset 0x10 (Segmento 2)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; "Far jump" para atualizar o registro cs com 0x08 (Segmento 1)
    jmp 0x08:.flush
.flush:
    ret