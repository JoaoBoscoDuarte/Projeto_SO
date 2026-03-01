; Faz as funções utilizáveis para o C
global outb
global inb

; outb - envia um byte para uma porta I/O
; stack: [esp + 8] data
;        [esp + 4] port
;        [esp    ] return address
outb:
    mov al, [esp + 8]
    mov dx, [esp + 4]
    out dx, al
    ret

; inb - lê um byte de uma porta I/O
; stack: [esp + 4] port
;        [esp    ] return address
inb:
    mov dx, [esp + 4]
    in al, dx
    ret
