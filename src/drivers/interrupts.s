global idt_load
global interrupt_handler_33
extern keyboard_handler_c

; Função para carregar a IDT
idt_load:
    mov eax, [esp + 4] ; Pega o ponteiro que passamos lá no C
    lidt [eax]         ; Comando x86 Load IDT
    ; sti                ; IMPORTANTE: Seta a flag de interrupção (habilita!)
    ret

; O Handler específico da interrupção 33 (Teclado)
extern pic_acknowledgement ; Vamos precisar avisar o PIC

interrupt_handler_33:
    pushad          ; Salva EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    
    ; Garante que o segmento de dados está correto (0x10 é o padrão da GDT para dados)
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call keyboard_handler_c
    
    ; --- ESSENCIAL: Avisar o PIC que a interrupção foi tratada ---
    ; Se você tiver uma função pic_send_eoi(33) no C, chame ela aqui.
    ; Ou faça manualmente via porta IO para testar:
    mov al, 0x20
    out 0x20, al    ; Envia EOI para o PIC mestre
    ; ------------------------------------------------------------

    popad
    iretd           ; Retorno oficial de interrupção 32 bits