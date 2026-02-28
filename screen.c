#include "types.h"

static volatile uint16_t* vga = (uint16_t*)0xB8000;
static uint8_t row = 0;
static uint8_t col = 0;

static void putc(char c)
{
    if (c == '\n') {
        row++;
        col = 0;
        return;
    }

    vga[row * 80 + col] = (uint16_t)c | (0x0F << 8);
    col++;

    if (col >= 80) {
        col = 0;
        row++;
    }
}

void kprint(const char* str)
{
    while (*str)
        putc(*str++);
}