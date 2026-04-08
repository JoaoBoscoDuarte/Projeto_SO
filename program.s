; =============================================================================
; program.s — Módulo de teste carregado pelo GRUB como Multiboot module
;
; PROPÓSITO:
;   Este binário é carregado pelo GRUB como um "módulo Multiboot" separado
;   do kernel. O kmain() recebe o endereço físico deste módulo via
;   multiboot_info_t.mods_addr e pode executá-lo diretamente.
;
; FORMATO:
;   Compilado como binário plano (-f bin), sem cabeçalho ELF.
;   O GRUB carrega os bytes brutos em memória e o kernel salta para
;   o primeiro byte — que deve ser uma instrução válida.
;
; COMO É CARREGADO:
;   No Makefile:
;     nasm -f bin program.s -o build/program
;   Na ISO:
;     grub.cfg: module /modules/program
;   No kmain.c:
;     unsigned int module_virt = mods[0].mod_start + 0xC0000000;
;     call_module_t start = (call_module_t) module_virt;
;     start();
;
; VERIFICAÇÃO EM RUNTIME:
;   - EAX = 0xDEADBEEF confirma que o módulo executou (visível no log serial)
;   - ECX = 0x1B confirma execução em ring 3 (CS com RPL=3)
;   - ECX = 0x08 indicaria ring 0 (CS com RPL=0)
;
; =============================================================================

[BITS 32]

    ; Marcador visível no log do emulador e na porta serial.
    ; Facilita confirmar que o módulo foi carregado e executado.
    mov eax, 0xDEADBEEF

    ; Lê o seletor de código atual (CS) para verificar o nível de privilégio.
    ;   CS = 0x1B (RPL=3) → executando em ring 3 (user mode)
    ;   CS = 0x08 (RPL=0) → executando em ring 0 (kernel mode)
    mov ecx, cs

    ; Loop infinito (não há para onde retornar).
    ; O kernel não espera retorno deste módulo.
.loop:
    jmp .loop
