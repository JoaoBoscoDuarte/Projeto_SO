#include "io.h"
#include "fb.h"

#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT    0x3D5
#define FB_HIGH_BYTE    14
#define FB_LOW_BYTE     15

static char *fb = (char *) 0x000B8000;
static unsigned short cursor_pos = 0;

void fb_clear(void)
{
    unsigned int i;
    for (i = 0; i < 80 * 25; i++) {
        fb[i * 2] = ' ';
        fb[i * 2 + 1] = ((FB_BLACK & 0x0F) << 4) | (FB_WHITE & 0x0F);
    }
    cursor_pos = 0;
    fb_move_cursor(cursor_pos);
}

void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    fb[i * 2] = c;
    fb[i * 2 + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

void fb_move_cursor(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE);
    outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE);
    outb(FB_DATA_PORT, pos & 0x00FF);
}

int fb_write(char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            cursor_pos = (cursor_pos / 80 + 1) * 80;
        } else if (buf[i] == '\r') {
            cursor_pos = (cursor_pos / 80) * 80;
        } else {
            fb_write_cell(cursor_pos, buf[i], FB_WHITE, FB_BLACK);
            cursor_pos++;
        }
        
        if (cursor_pos >= 80 * 25) {
            cursor_pos = 0;
        }
    }
    fb_move_cursor(cursor_pos);
    return (int)len;
}
