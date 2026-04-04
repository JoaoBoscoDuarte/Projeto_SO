#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

/* ============================================================================
 * keyboard.h — Driver de teclado PS/2 com buffer circular
 *
 * A ISR (keyboard_handler_c) deposita caracteres ASCII no buffer circular.
 * O shell consome via kbd_getchar() / kbd_readline().
 * ========================================================================== */

/* Tamanho do buffer circular de teclas (deve ser potência de 2) */
#define KBD_BUFFER_SIZE 256

/*
 * keyboard_handler_c — chamada pela ISR do teclado (IRQ1 / interrupção 33).
 * Lê o scancode da porta 0x60, converte para ASCII e deposita no buffer.
 * NÃO escreve diretamente no framebuffer.
 */
void keyboard_handler_c(void);

/*
 * kbd_getchar — retorna o próximo caractere disponível no buffer.
 * Bloqueia (polling com hlt) enquanto o buffer estiver vazio.
 * Deve ser chamada apenas de código que pode ser interrompido (sti ativo).
 */
char kbd_getchar(void);

/*
 * kbd_try_getchar — retorna o próximo caractere do buffer sem bloquear.
 * Retorna 0 se o buffer estiver vazio.
 */
char kbd_try_getchar(void);

/*
 * kbd_readline — lê uma linha de texto até '\n' ou max_len-1 caracteres.
 * Faz eco dos caracteres no framebuffer via fb_putchar().
 * Trata backspace ('\b') removendo o último caractere.
 * Retorna o número de caracteres lidos (sem o '\0' final).
 */
int kbd_readline(char *buf, unsigned int max_len);

#endif /* INCLUDE_KEYBOARD_H */
