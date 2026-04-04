; ============================================================================
; tss.s — Carregamento do Task State Segment
; ============================================================================
; O registro TR (Task Register) do x86 aponta para o TSS ativo.
; A instrução 'ltr' carrega o seletor do descritor TSS da GDT no TR.
;
; SEL_TSS = GDT_TSS | RPL3 = 0x28 | 0x3 = 0x2B
;
; Nota: 'ltr' só pode ser executada em ring 0.
; Deve ser chamada APÓS gdt_flush() ter carregado a nova GDT com o
; descritor TSS no índice 5.
; ============================================================================

global tss_flush

tss_flush:
    mov ax, 0x2B        ; seletor TSS: índice 5 (0x28) com RPL=3 (0x2B)
    ltr ax              ; carrega o TSS no Task Register
    ret
