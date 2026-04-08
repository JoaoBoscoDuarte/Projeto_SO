; =============================================================================
; usermode.s — Transição ring 0 → ring 3 via IRET
;
; PROPÓSITO:
;   Transfere a execução para um processo de usuário (ring 3) a partir
;   do kernel (ring 0). Esta transição não pode ser feita com um simples
;   JMP ou CALL — o x86 exige o uso de IRET para mudar o nível de privilégio.
;
; MECANISMO DO IRET COM TROCA DE PRIVILÉGIO:
;   Quando IRET detecta que o CS na stack tem RPL > CPL atual, ele executa
;   uma troca de privilégio completa, desempilhando 5 valores (em vez de 3):
;
;     [esp+0]  EIP    → próxima instrução a executar em ring 3
;     [esp+4]  CS     → seletor de código com RPL=3 (0x1B)
;     [esp+8]  EFLAGS → flags do processador (IF=1 para habilitar interrupções)
;     [esp+12] ESP    → stack pointer do processo user
;     [esp+16] SS     → seletor de dados com RPL=3 (0x23)
;
;   Após o IRET:
;     - CPU passa a executar em ring 3
;     - ESP aponta para a user stack
;     - Interrupções habilitadas (IF=1 em EFLAGS)
;
; SELETORES GDT USADOS:
;   0x1B = GDT[3] | RPL3 = 0x18 | 0x3  → código user (ring 3, execute/read)
;   0x23 = GDT[4] | RPL3 = 0x20 | 0x3  → dados user  (ring 3, read/write)
;
; NOTA SOBRE TSS:
;   Quando uma interrupção ocorrer em ring 3, a CPU consultará o TSS para
;   saber qual kernel stack usar (ss0/esp0). O TSS deve estar configurado
;   antes de chamar enter_usermode — feito em tss_init() no kmain.
; =============================================================================

global enter_usermode

; -----------------------------------------------------------------------------
; enter_usermode(eip, esp, page_directory_phys)
;
; Parâmetros (convenção cdecl — lidos da stack):
;   [esp+4]  eip                — entry point do programa user
;   [esp+8]  esp                — topo da stack do processo user
;   [esp+12] page_directory_phys — endereço FÍSICO do page directory do processo
;
; Esta função NÃO retorna — o controle é transferido para ring 3 via IRET.
; -----------------------------------------------------------------------------
enter_usermode:
    ; Desabilita interrupções durante a montagem do frame de IRET.
    ; Serão reabilitadas automaticamente pelo IRET ao carregar EFLAGS com IF=1.
    cli

    ; -------------------------------------------------------------------------
    ; Passo 1: Troca o page directory para o do processo
    ;
    ; Escreve o endereço físico do PD do processo em CR3.
    ; Isso invalida todo o TLB — necessário porque o novo PD mapeia um
    ; espaço de endereçamento diferente do kernel.
    ; -------------------------------------------------------------------------
    mov eax, [esp+12]               ; page_directory_phys
    mov cr3, eax

    ; -------------------------------------------------------------------------
    ; Passo 2: Atualiza os segment registers de dados para ring 3
    ;
    ; DS, ES, FS, GS precisam ter RPL=3 antes do IRET.
    ; O SS será configurado pelo próprio IRET via stack.
    ; 0x23 = seletor de dados user (GDT[4] | RPL3).
    ; -------------------------------------------------------------------------
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; -------------------------------------------------------------------------
    ; Passo 3: Salva os argumentos antes de modificar ESP
    ;
    ; Os PUSHes a seguir modificam ESP, então lemos os argumentos agora.
    ; -------------------------------------------------------------------------
    mov eax, [esp+8]                ; user esp
    mov ebx, [esp+4]                ; user eip

    ; -------------------------------------------------------------------------
    ; Passo 4: Monta o frame de retorno do IRET
    ;
    ; IRET lê da stack na ordem: EIP, CS, EFLAGS, ESP, SS.
    ; Empilhamos na ordem INVERSA (último a ser lido = primeiro a ser empilhado).
    ;
    ; EFLAGS = 0x202:
    ;   bit 1 (reservado) = 1  — sempre deve ser 1
    ;   bit 9 (IF)        = 1  — habilita interrupções em ring 3
    ; -------------------------------------------------------------------------
    push dword 0x23                 ; SS     — user data selector (RPL=3)
    push eax                        ; ESP    — user stack pointer
    push dword 0x202                ; EFLAGS — IF=1, bit1=1
    push dword 0x1B                 ; CS     — user code selector (RPL=3)
    push ebx                        ; EIP    — user entry point

    ; -------------------------------------------------------------------------
    ; Passo 5: Executa IRET — transfere controle para ring 3
    ;
    ; Como CS.RPL (3) > CPL (0), IRET executa troca de privilégio:
    ;   1. Carrega EIP e CS (verifica DPL na GDT)
    ;   2. Carrega EFLAGS (habilita interrupções via IF=1)
    ;   3. Carrega ESP e SS (troca para a user stack)
    ;   4. CPU passa a executar em ring 3
    ; -------------------------------------------------------------------------
    iret
