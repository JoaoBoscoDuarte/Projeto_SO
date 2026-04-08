; switch.s — Troca de contexto cooperativa entre processos kernel
;
; Convenção cdecl:
;   context_switch(unsigned int *old_esp_ptr, unsigned int new_esp)
;
; O que a função faz:
;   1. Salva os registradores callee-saved do processo atual na sua kernel stack
;   2. Grava o ESP atual em *old_esp_ptr (campo proc->esp no PCB)
;   3. Carrega new_esp como novo stack pointer
;   4. Restaura os registradores callee-saved do novo processo
;   5. Retorna — o "ret" volta para onde o novo processo estava quando foi suspenso
;
; Após context_switch retornar no contexto do novo processo, a execução continua
; de onde ele estava (logo após o seu próprio "call context_switch").

section .text
global context_switch

context_switch:
    ; Salva registradores callee-saved do processo atual
    push ebp
    push ebx
    push esi
    push edi

    ; Salva ESP atual em *old_esp_ptr
    ; old_esp_ptr está em [esp + 20]: 4 pushes × 4 bytes + endereço de retorno
    mov eax, [esp + 20]
    mov [eax], esp

    ; Carrega o stack pointer do novo processo
    ; new_esp está em [esp + 24]
    mov esp, [esp + 24]

    ; Restaura registradores callee-saved do novo processo
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
