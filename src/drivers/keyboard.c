#include "keyboard.h"
#include "fb.h"
#include "scheduler.h"

extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);

/* ============================================================================
 * Mapa de scancodes US QWERTY → ASCII (make codes apenas; ignore 0x80+)
 * ============================================================================
 *
 * OBS:
 * Este mapa continua simples e baseado em layout US.
 * As teclas de seta continuam sendo tratadas separadamente via prefixo E0.
 * ========================================================================== */
static unsigned char kbd_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   '*', 0,   ' '
};

/* ============================================================================
 * Buffer circular de caracteres
 *
 * head: próxima posição de escrita (produzida pela ISR)
 * tail: próxima posição de leitura (consumida por kbd_getchar)
 * Buffer cheio quando (head+1) % SIZE == tail.
 * ========================================================================== */
static volatile char         kbd_buffer[KBD_BUFFER_SIZE];
static volatile unsigned int kbd_head = 0;
static volatile unsigned int kbd_tail = 0;

/* ============================================================================
 * Prefixo de scancode estendido
 *
 * Quando chega 0xE0, a próxima leitura representa uma tecla estendida,
 * como as setas.
 * ========================================================================== */
static volatile unsigned char e0_prefix = 0;

static void kbd_buffer_put(char c)
{
    unsigned int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
    /* Se o buffer estiver cheio, o caractere é silenciosamente descartado */
}

/* ============================================================================
 * keyboard_handler_c — ISR chamada pela interrupção 33 (IRQ1)
 *
 * Lê o scancode, converte para ASCII, e deposita no buffer circular.
 * Não faz acesso ao framebuffer.
 * ========================================================================== */
void keyboard_handler_c(void)
{
    unsigned char scancode = inb(0x60);


    // Detecta prefixo E0
    if (scancode == 0xE0) {
        e0_prefix = 1;
        return;
    }

    // Se estava aguardando o segundo byte de um scancode estendido 
    if (e0_prefix) {
        e0_prefix = 0;

        // ignora releases estendidos 
        if (scancode & 0x80)
            return;

        switch (scancode) {
            case 0x48: // seta para cima 
                kbd_buffer_put(27);  // ESC 
                kbd_buffer_put('[');
                kbd_buffer_put('A');
                return;

            case 0x50: // seta para baixo 
                kbd_buffer_put(27);  // ESC
                kbd_buffer_put('[');
                kbd_buffer_put('B');
                return;

            default:
                return;
        }
    }

    /* Ignora break codes (bit 7 = 1) */
    if (scancode & 0x80)
        return;

    if (scancode < 128) {
        char c = (char)kbd_map[scancode];

        if (c == 0)
        return;

        /* comportamento normal: tecla comum vai direto para o buffer */
        kbd_buffer_put((char)c);

    }
}

/* ============================================================================
 * kbd_getchar — bloqueia até ter um caractere disponível
 * ========================================================================== */
char kbd_getchar(void)
{
    while (kbd_tail == kbd_head) {
        yield();
        asm volatile("hlt");
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

char kbd_try_getchar(void)
{
    if (kbd_tail == kbd_head)
        return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

/* ============================================================================
 * kbd_readline — lê uma linha com eco no framebuffer
 * ========================================================================== */
int kbd_readline(char *buf, unsigned int max_len)
{
    unsigned int i = 0;

    if (!buf || max_len == 0)
        return 0;

    while (i < max_len - 1) {
        char c = kbd_getchar();

        if (c == '\n') {
            buf[i] = '\0';
            fb_putchar('\n');
            return (int)i;
        }

        if (c == '\b') {
            if (i > 0) {
                i--;
                fb_putchar('\b');
            }
            continue;
        }

        buf[i++] = c;
        fb_putchar(c);
    }

    buf[i] = '\0';
    return (int)i;
}
