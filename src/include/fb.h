#ifndef INCLUDE_FB_H
#define INCLUDE_FB_H

/* ============================================================================
 * fb.h — Driver de framebuffer VGA (modo texto 80x25)
 * ========================================================================== */

/* Constantes de layout */
#define FB_COLS  80
#define FB_ROWS  25

/* Cores VGA */
#define FB_BLACK         0
#define FB_BLUE          1
#define FB_GREEN         2
#define FB_CYAN          3
#define FB_RED           4
#define FB_MAGENTA       5
#define FB_BROWN         6
#define FB_LIGHT_GREY    7
#define FB_DARK_GREY     8
#define FB_LIGHT_BLUE    9
#define FB_LIGHT_GREEN   10
#define FB_LIGHT_CYAN    11
#define FB_LIGHT_RED     12
#define FB_LIGHT_MAGENTA 13
#define FB_LIGHT_BROWN   14
#define FB_WHITE         15

/* --- API original --- */

/* Limpa a tela inteira */
void fb_clear(void);

/* Escreve um caractere na posição linear i (0–1999) */
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg);

/* Move o cursor piscante do hardware */
void fb_move_cursor(unsigned short pos);

/* Escreve len bytes de buf na posição atual do cursor (com newline/scroll) */
int fb_write(char *buf, unsigned int len);

/* Rola a tela uma linha para cima */
void fb_scroll(void);

/* --- API posicional (nova) --- */

/*
 * fb_write_at — escreve um caractere em (row, col) com cores explícitas.
 * Não altera o cursor nem provoca scroll.
 */
void fb_write_at(unsigned int row, unsigned int col,
                 char c, unsigned char fg, unsigned char bg);

/*
 * fb_write_str_at — escreve uma string a partir de (row, col).
 * Para na primeira coluna além de FB_COLS ou no '\0'.
 * Não altera o cursor nem provoca scroll.
 */
void fb_write_str_at(unsigned int row, unsigned int col,
                     const char *str, unsigned char fg, unsigned char bg);

/*
 * fb_clear_line — limpa toda a linha row com espaços (fundo preto).
 */
void fb_clear_line(unsigned int row);

/*
 * fb_putchar — escreve c na posição atual do cursor e avança.
 * Suporta '\n' (nova linha), '\b' (backspace com apagamento) e scroll.
 * Atualiza o cursor do hardware.
 */
void fb_putchar(char c);

/*
 * fb_set_cursor — posiciona o cursor em (row, col).
 * Atualiza o cursor do hardware.
 */
void fb_set_cursor(unsigned int row, unsigned int col);

/* Retorna linha e coluna atuais do cursor */
unsigned int fb_get_cursor_row(void);
unsigned int fb_get_cursor_col(void);

#endif /* INCLUDE_FB_H */
