#ifndef INCLUDE_UTF8_H
#define INCLUDE_UTF8_H

// Converte string UTF-8 para Code Page 437
int utf8_to_cp437_convert(const char *str, unsigned char *out, int max_len);

// Escreve string UTF-8 no framebuffer
void fb_write_utf8(const char *str);

// Converte um caractere UTF-8 para CP437
unsigned char utf8_char_to_cp437(const unsigned char *utf8_bytes, int *bytes_consumed);

#endif
