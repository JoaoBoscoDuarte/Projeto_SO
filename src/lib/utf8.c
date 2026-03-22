// ============================================================================
// UTF8.C - Conversor UTF-8 para Code Page 437
// ============================================================================
// UTF-8 usa múltiplos bytes para caracteres especiais
// VGA só aceita 1 byte por caractere, então convertemos para CP437
// ============================================================================

#include "fb.h"
#include "utf8.h"

// ============================================================================
// Tabela de conversão UTF-8 → Code Page 437
// ============================================================================
// Estrutura: {byte1_utf8, byte2_utf8, cp437}
struct utf8_to_cp437 {
    unsigned char utf8_byte1;
    unsigned char utf8_byte2;
    unsigned char cp437;
};

// Tabela de conversão para caracteres comuns em português
static struct utf8_to_cp437 utf8_table[] = {
    // Vogais minúsculas acentuadas
    {0xC3, 0xA1, 0xA0},  // á
    {0xC3, 0xA0, 0x85},  // à
    {0xC3, 0xA2, 0x83},  // â
    {0xC3, 0xA3, 0xC6},  // ã
    {0xC3, 0xA9, 0x82},  // é
    {0xC3, 0xAA, 0x88},  // ê
    {0xC3, 0xAD, 0xA1},  // í
    {0xC3, 0xB3, 0xA2},  // ó
    {0xC3, 0xB4, 0x93},  // ô
    {0xC3, 0xB5, 0xE4},  // õ
    {0xC3, 0xBA, 0xA3},  // ú
    {0xC3, 0xBC, 0x81},  // ü
    
    // Vogais maiúsculas acentuadas
    {0xC3, 0x81, 0xB5},  // Á
    {0xC3, 0x80, 0xB7},  // À
    {0xC3, 0x82, 0xB6},  // Â
    {0xC3, 0x89, 0x90},  // É
    {0xC3, 0x8A, 0xD2},  // Ê
    {0xC3, 0x8D, 0xD6},  // Í
    {0xC3, 0x93, 0xE0},  // Ó
    {0xC3, 0x94, 0xE2},  // Ô
    {0xC3, 0x9A, 0xE9},  // Ú
    
    // Cedilha
    {0xC3, 0xA7, 0x87},  // ç
    {0xC3, 0x87, 0x80},  // Ç
    
    // Fim da tabela
    {0, 0, 0}
};

// ============================================================================
// utf8_char_to_cp437 - Converte um caractere UTF-8 para CP437
// ============================================================================
// utf8_bytes: ponteiro para bytes UTF-8
// bytes_consumed: retorna quantos bytes foram consumidos (1, 2, 3 ou 4)
// Retorna: caractere em CP437
unsigned char utf8_char_to_cp437(const unsigned char *utf8_bytes, int *bytes_consumed) {
    unsigned char byte1 = utf8_bytes[0];
    
    // Caractere ASCII simples (0x00-0x7F)
    if (byte1 < 0x80) {
        *bytes_consumed = 1;
        return byte1;
    }
    
    // Caractere UTF-8 de 2 bytes (0xC0-0xDF)
    if ((byte1 & 0xE0) == 0xC0) {
        unsigned char byte2 = utf8_bytes[1];
        *bytes_consumed = 2;
        
        // Procura na tabela de conversão
        for (int j = 0; utf8_table[j].utf8_byte1 != 0; j++) {
            if (utf8_table[j].utf8_byte1 == byte1 && 
                utf8_table[j].utf8_byte2 == byte2) {
                return utf8_table[j].cp437;
            }
        }
        
        // Se não encontrou, usa '?'
        return '?';
    }
    
    // Caractere UTF-8 de 3 bytes (0xE0-0xEF) - não suportado
    if ((byte1 & 0xF0) == 0xE0) {
        *bytes_consumed = 3;
        return '?';
    }
    
    // Caractere UTF-8 de 4 bytes (0xF0-0xF7) - não suportado
    if ((byte1 & 0xF8) == 0xF0) {
        *bytes_consumed = 4;
        return '?';
    }
    
    // Byte inválido
    *bytes_consumed = 1;
    return '?';
}

// ============================================================================
// utf8_to_cp437_convert - Converte sequência UTF-8 para Code Page 437
// ============================================================================
// str: string UTF-8 de entrada
// out: buffer de saída em CP437
// max_len: tamanho máximo do buffer de saída
// Retorna: número de caracteres escritos
int utf8_to_cp437_convert(const char *str, unsigned char *out, int max_len) {
    int i = 0;      // Índice na string UTF-8
    int out_idx = 0; // Índice no buffer de saída
    
    while (str[i] != '\0' && out_idx < max_len - 1) {
        int bytes_consumed = 0;
        unsigned char cp437_char = utf8_char_to_cp437((const unsigned char*)&str[i], &bytes_consumed);
        out[out_idx++] = cp437_char;
        i += bytes_consumed;
    }
    
    out[out_idx] = '\0';
    return out_idx;
}

// ============================================================================
// fb_write_utf8 - Escreve string UTF-8 no framebuffer
// ============================================================================
// Converte UTF-8 para CP437 antes de escrever
void fb_write_utf8(const char *str) {
    unsigned char buffer[256];
    int len = utf8_to_cp437_convert(str, buffer, 256);
    fb_write((char*)buffer, len);
}
