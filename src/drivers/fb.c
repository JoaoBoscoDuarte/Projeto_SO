#include "io.h"
#include "fb.h"

#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT    0x3D5
#define FB_HIGH_BYTE    14
#define FB_LOW_BYTE     15

static char *fb = (char *) 0x000B8000;
static unsigned short cursor_pos = 0;

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
        fb_write_cell(cursor_pos, buf[i], FB_WHITE, FB_BLACK);
        cursor_pos++;
        if (cursor_pos >= 80 * 25) {
            cursor_pos = 0;
        }
    }
    fb_move_cursor(cursor_pos);
    return (int)len;
}