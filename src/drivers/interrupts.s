; ============================================================================
; interrupts.s — Handlers de interrupção
; ============================================================================
;
; SUPORTE A RING 3 (Capítulo 11):
;
; Quando uma interrupção ocorre em ring 3 (user mode), o CPU:
;   1. Lê ss0/esp0 do TSS e troca para a kernel stack
;   2. Pusha ss_user, esp_user, eflags, cs_user, eip_user na kernel stack
;   3. Salta para o handler da IDT
;
; Ao entrar no handler, DS e ES ainda têm o valor do user (0x23).
; Precisamos:
;   a) Salvar DS e ES na stack (para restaurar ao sair)
;   b) Setar DS e ES para o seletor de dados do kernel (0x10)
;   c) Processar a interrupção
;   d) Restaurar DS e ES originais
;   e) Executar iretd — que restaura CS, EIP, EFLAGS e, se houve troca
;      de privilégio, também SS e ESP (volta para ring 3 automaticamente)
; ============================================================================

global idt_load
global interrupt_handler_32
global interrupt_handler_33
extern pit_handler_c
extern keyboard_handler_c

; idt_load — carrega a IDT usando o ponteiro passado do C
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; ============================================================================
; interrupt_handler_32 — IRQ0 (timer PIT, remapeado para vetor 32)
; ============================================================================
interrupt_handler_32:
    pushad
    push ds
    push es
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    call pit_handler_c
    mov al, 0x20
    out 0x20, al
    pop es
    pop ds
    popad
    iretd

; ============================================================================
; interrupt_handler_33 — IRQ1 (teclado, remapeado para vetor 33)
; ============================================================================
interrupt_handler_33:
    ; Salva todos os registradores gerais.
    ; pushad salva: EAX ECX EDX EBX ESP EBP ESI EDI (nesta ordem, 32 bytes)
    pushad

    ; Salva DS e ES originais antes de sobrescrever.
    ; Se a interrupção veio de ring 3, eles valem 0x23 (user data).
    ; Se veio de ring 0, valem 0x10 (kernel data).
    ; Em ambos os casos, restauramos o valor correto ao sair.
    push ds
    push es

    ; Seta DS e ES para o seletor de dados do kernel (0x10).
    ; Necessário para que o código C do handler acesse o kernel corretamente.
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; Chama o handler C do teclado
    call keyboard_handler_c

    ; Envia EOI (End Of Interrupt) ao PIC mestre.
    ; Sem EOI, o PIC não gera mais interrupções do IRQ1 (teclado).
    mov al, 0x20
    out 0x20, al

    ; Restaura DS e ES originais (0x23 se veio de ring 3, 0x10 se de ring 0)
    pop es
    pop ds

    ; Restaura os registradores gerais
    popad

    ; Retorna da interrupção.
    ; iretd desempilha: EIP, CS, EFLAGS.
    ; Se CS.RPL > CPL (troca de privilégio: ring 0 → ring 3), também
    ; desempilha ESP e SS — voltando para a user stack automaticamente.
    iretd
