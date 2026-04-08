#include "io.h"
#include "fb.h"

/* ============================================================================
 * fb.c — Driver de framebuffer VGA (modo texto 80x25)
 *
 * Memória VGA: 0xB8000 (mapeado em alto-endereço em 0xC00B8000 pelo kernel)
 * Layout de cada célula: [byte0 = char][byte1 = atributo de cor]
 * atributo: bits 7-4 = fundo, bits 3-0 = texto
 * ========================================================================== */

#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT    0x3D5
#define FB_HIGH_BYTE    14
#define FB_LOW_BYTE     15

static char *fb = (char *)0xC00B8000;

/* cursor_pos: posição linear 0..(FB_COLS*FB_ROWS - 1) */
static unsigned short cursor_pos = 0;

/* ============================================================================
 * Funções originais
 * ========================================================================== */

void fb_clear(void)
{
    unsigned int i;
    for (i = 0; i < FB_COLS * FB_ROWS; i++) {
        fb[i * 2]     = ' ';
        fb[i * 2 + 1] = ((FB_BLACK & 0x0F) << 4) | (FB_LIGHT_GREY & 0x0F);
    }
    cursor_pos = 0;
    fb_move_cursor(cursor_pos);
}

void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    fb[i * 2]     = c;
    fb[i * 2 + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

void fb_move_cursor(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE);
    outb(FB_DATA_PORT,    (pos >> 8) & 0x00FF);
    outb(FB_COMMAND_PORT, FB_LOW_BYTE);
    outb(FB_DATA_PORT,    pos & 0x00FF);
}

int fb_write(char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            cursor_pos = (unsigned short)((cursor_pos / FB_COLS + 1) * FB_COLS);
        } else if (buf[i] == '\r') {
            cursor_pos = (unsigned short)((cursor_pos / FB_COLS) * FB_COLS);
        } else {
            fb_write_cell(cursor_pos, buf[i], FB_LIGHT_GREY, FB_BLACK);
            cursor_pos++;
        }
        if (cursor_pos >= FB_COLS * FB_ROWS) {
            fb_scroll();
            cursor_pos = (unsigned short)(FB_COLS * (FB_ROWS - 1));
        }
    }
    fb_move_cursor(cursor_pos);
    return (int)len;
}

void fb_scroll(void)
{
    unsigned int i;
    for (i = 0; i < FB_COLS * (FB_ROWS - 1) * 2; i++)
        fb[i] = fb[i + FB_COLS * 2];
    for (i = FB_COLS * (FB_ROWS - 1) * 2; i < FB_COLS * FB_ROWS * 2; i += 2) {
        fb[i]     = ' ';
        fb[i + 1] = 0x07;
    }
}

/* ============================================================================
 * Funções posicionais novas
 * ========================================================================== */

void fb_write_at(unsigned int row, unsigned int col,
                 char c, unsigned char fg, unsigned char bg)
{
    if (row >= FB_ROWS || col >= FB_COLS)
        return;
    unsigned int pos = row * FB_COLS + col;
    fb[pos * 2]     = c;
    fb[pos * 2 + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

void fb_write_str_at(unsigned int row, unsigned int col,
                     const char *str, unsigned char fg, unsigned char bg)
{
    unsigned int c = col;
    if (row >= FB_ROWS)
        return;
    while (*str && c < FB_COLS) {
        fb_write_at(row, c, *str, fg, bg);
        str++;
        c++;
    }
}

void fb_clear_line(unsigned int row)
{
    unsigned int col;
    if (row >= FB_ROWS)
        return;
    for (col = 0; col < FB_COLS; col++)
        fb_write_at(row, col, ' ', FB_LIGHT_GREY, FB_BLACK);
}

void fb_putchar(char c)
{
    if (c == '\n') {
        cursor_pos = (unsigned short)((cursor_pos / FB_COLS + 1) * FB_COLS);
    } else if (c == '\b') {
        if (cursor_pos > 0) {
            cursor_pos--;
            fb_write_cell(cursor_pos, ' ', FB_LIGHT_GREY, FB_BLACK);
        }
    } else {
        fb_write_cell(cursor_pos, c, FB_LIGHT_GREY, FB_BLACK);
        cursor_pos++;
    }
    if (cursor_pos >= FB_COLS * FB_ROWS) {
        fb_scroll();
        cursor_pos = (unsigned short)(FB_COLS * (FB_ROWS - 1));
    }
    fb_move_cursor(cursor_pos);
}

void fb_set_cursor(unsigned int row, unsigned int col)
{
    if (row >= FB_ROWS) row = FB_ROWS - 1;
    if (col >= FB_COLS) col = FB_COLS - 1;
    cursor_pos = (unsigned short)(row * FB_COLS + col);
    fb_move_cursor(cursor_pos);
}

unsigned int fb_get_cursor_row(void)
{
    return cursor_pos / FB_COLS;
}

unsigned int fb_get_cursor_col(void)
{
    return cursor_pos % FB_COLS;
}
