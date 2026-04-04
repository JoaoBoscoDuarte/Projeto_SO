; ============================================================================
; usermode.s — Transição ring 0 → ring 3 via iret
; ============================================================================
;
; Para fazer a transição para user mode, o x86 usa a instrução 'iret'.
; Ela lê da stack (de cima para baixo):
;
;   [esp+0]  eip    → instruction pointer do user
;   [esp+4]  cs     → code segment selector com RPL=3
;   [esp+8]  eflags → flags (IF=1 para habilitar interrupções)
;   [esp+12] esp    → stack pointer do user
;   [esp+16] ss     → stack segment selector com RPL=3
;
; Quando iret detecta que cs tem RPL > CPL (troca de privilégio), ele:
;   1. Carrega EIP e CS (com DPL=3 verificado na GDT)
;   2. Carrega EFLAGS
;   3. Carrega ESP e SS do user (restaura a user stack)
;   4. A CPU passa a executar em ring 3
;
; Seletores:
;   SEL_USER_CODE = GDT[3] | RPL3 = 0x18 | 0x3 = 0x1B
;   SEL_USER_DATA = GDT[4] | RPL3 = 0x20 | 0x3 = 0x23
; ============================================================================

global enter_usermode

; enter_usermode(eip, esp, page_directory_phys)
;
; Convenção cdecl — argumentos na stack:
;   [esp+4]  = eip              (entry point do user program)
;   [esp+8]  = esp              (user stack pointer)
;   [esp+12] = page_directory_phys (endereço FÍSICO do page directory)
;
; Esta função NÃO retorna — iret transfere o controle para ring 3.

enter_usermode:
    ; Desabilita interrupções durante a troca de contexto.
    ; Serão reabilitadas pelo iret ao carregar eflags com IF=1.
    cli

    ; --- 1. Troca o page directory para o do processo ---
    ; Lê o terceiro argumento (page_directory_phys) e escreve em CR3.
    ; Isso invalida todo o TLB — necessário pois o novo PD mapeia
    ; o espaço de endereçamento do processo.
    mov eax, [esp+12]
    mov cr3, eax

    ; --- 2. Atualiza os segment registers de dados para user mode ---
    ; DS, ES, FS e GS precisam ser setados para o seletor user data (0x23)
    ; antes do iret. O SS será configurado pelo próprio iret via stack.
    mov ax, 0x23            ; SEL_USER_DATA = 0x20 | RPL3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- 3. Salva os argumentos antes de modificar ESP ---
    mov eax, [esp+8]        ; user esp
    mov ebx, [esp+4]        ; user eip

    ; --- 4. Monta o frame de retorno do iret na stack atual ---
    ; Ordem OBRIGATÓRIA (iret lê de cima para baixo):
    push dword 0x23         ; ss     — user data selector + RPL3
    push eax                ; esp    — user stack pointer
    push dword 0x202        ; eflags — bit1=reservado(1), bit9=IF(1) = 0x202
    push dword 0x1B         ; cs     — user code selector + RPL3
    push ebx                ; eip    — user entry point

    ; --- 5. Salta para ring 3 ---
    ; iret desempilha eip, cs, eflags, esp, ss nesta ordem.
    ; Como cs.RPL (3) > CPL (0), ocorre troca de privilégio:
    ;   - SS e ESP são carregados do stack (restaura user stack)
    ;   - CPU passa a executar em ring 3
    iret
