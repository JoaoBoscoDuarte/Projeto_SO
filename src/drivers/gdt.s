global gdt_flush

gdt_flush:
    ; Pega o endereço da estrutura 'gp' que enviamos do C
    mov eax, [esp + 4]  
    
    ; O comando LGDT (Load GDT) diz ao processador usar a tabela
    lgdt [eax]          

    ; Agora precisamos atualizar os "registradores de segmento".
    ; Eles são como atalhos no CPU que precisam apontar para o crachá 2 (Dados).
    ; 0x10 é o endereço da Entrada 2 (cada entrada tem 8 bytes, então 2 * 8 = 16, ou 0x10 em hexadecimal).
    mov ax, 0x10        
    mov ds, ax          ; Atualiza Segmento de Dados
    mov es, ax          ; Atualiza Segmento Extra
    mov fs, ax          ; Atualiza outro Extra
    mov gs, ax          ; Atualiza mais um Extra
    mov ss, ax          ; Atualiza a Pilha (onde as funções rodam)

    ; O Segmento de Código (CS) é especial. Não dá para usar 'mov'.
    ; Temos que dar um "pulo" (jump) para forçar o CPU a recarregar o CS.
    ; 0x08 aponta para a Entrada 1 (1 * 8 = 8).
    jmp 0x08:.flush     

.flush:
    ret                 ; Pronto! O modo protegido está configurado.