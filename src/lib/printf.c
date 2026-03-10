#include "printf.h"
#include "fb.h"
#include "serial.h"

// ============================================================================
// PRINTF - Biblioteca de formatação de strings para o kernel
// ============================================================================
// Implementação simplificada de printf que suporta %s, %d, %x
// ============================================================================

// ============================================================================
// my_strlen - Calcula tamanho de uma string
// ============================================================================
static int my_strlen(const char *str)
{
    int len = 0;
    while (str[len]) len++;  // Conta até encontrar '\0'
    return len;
}

// ============================================================================
// itoa - Converte inteiro para string (Integer to ASCII)
// ============================================================================
// num: número a converter
// str: buffer de saída (deve ter espaço suficiente)
static void itoa(int num, char *str)
{
    int i = 0;
    int is_negative = 0;
    
    // Caso especial: zero
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Trata números negativos
    if (num < 0) {
        is_negative = 1;
        num = -num;  // Trabalha com valor positivo
    }
    
    // Extrai dígitos de trás para frente
    while (num != 0) {
        str[i++] = (num % 10) + '0';  // Converte dígito para caractere
        num = num / 10;
    }
    
    // Adiciona sinal negativo se necessário
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';  // Termina string
    
    // Inverte a string (estava de trás para frente)
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// ============================================================================
// itohex - Converte inteiro para hexadecimal
// ============================================================================
static void itohex(unsigned int num, char *str)
{
    const char hex[] = "0123456789ABCDEF";  // Tabela de conversão
    int i = 0;
    
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Extrai dígitos hexadecimais de trás para frente
    while (num != 0) {
        str[i++] = hex[num % 16];  // Pega dígito hex
        num = num / 16;
    }
    
    str[i] = '\0';
    
    // Inverte a string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// ============================================================================
// kprintf - Printf do kernel (suporta %s, %d, %x)
// ============================================================================
// device: onde enviar saída (framebuffer, serial, ou ambos)
// fmt: string de formato
// ...: argumentos variáveis
void kprintf(output_device_t device, const char *fmt, ...)
{
    char buffer[256];  // Buffer temporário
    int buf_idx = 0;
    
    // Acessa argumentos variáveis (implementação manual de va_args)
    int *args = (int*)((char*)&fmt + sizeof(fmt));
    int arg_idx = 0;
    int i;
    
    // Processa string de formato
    for (i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i + 1] != '\0') {
            i++;  // Pula o '%'
            
            if (fmt[i] == 's') {
                // %s: string
                char *s = (char*)args[arg_idx++];
                while (*s) {
                    buffer[buf_idx++] = *s++;
                }
                
            } else if (fmt[i] == 'd') {
                // %d: inteiro decimal
                char num_str[12];
                int j;
                itoa(args[arg_idx++], num_str);
                for (j = 0; num_str[j]; j++) {
                    buffer[buf_idx++] = num_str[j];
                }
                
            } else if (fmt[i] == 'x') {
                // %x: hexadecimal
                char hex_str[12];
                int j;
                itohex((unsigned int)args[arg_idx++], hex_str);
                for (j = 0; hex_str[j]; j++) {
                    buffer[buf_idx++] = hex_str[j];
                }
                
            } else {
                // Caractere desconhecido após %, copia literal
                buffer[buf_idx++] = fmt[i];
            }
        } else {
            // Caractere normal, copia para buffer
            buffer[buf_idx++] = fmt[i];
        }
    }
    
    // Envia buffer para dispositivo(s) de saída
    if (device == OUTPUT_FB || device == OUTPUT_BOTH) {
        fb_write(buffer, buf_idx);
    }
    if (device == OUTPUT_SERIAL || device == OUTPUT_BOTH) {
        serial_write(SERIAL_COM1_BASE, buffer, buf_idx);
    }
}

// ============================================================================
// log_debug - Envia mensagem de debug apenas para serial
// ============================================================================
// Útil para debug sem poluir a tela
void log_debug(const char *msg)
{
    int len = my_strlen(msg);
    serial_write(SERIAL_COM1_BASE, "[DEBUG] ", 8);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, "\n", 1);
}

// ============================================================================
// log_info - Envia mensagem informativa para tela e serial
// ============================================================================
void log_info(const char *msg)
{
    int len = my_strlen(msg);
    char prefix[] = "[INFO] ";
    char newline[] = "\n";
    
    // Envia para framebuffer (tela)
    fb_write(prefix, 7);
    fb_write((char*)msg, len);
    fb_write(newline, 1);
    
    // Envia para serial (arquivo de log)
    serial_write(SERIAL_COM1_BASE, prefix, 7);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, newline, 1);
}

// ============================================================================
// log_error - Envia mensagem de erro para tela e serial
// ============================================================================
void log_error(const char *msg)
{
    int len = my_strlen(msg);
    char prefix[] = "[ERROR] ";
    char newline[] = "\n";
    
    // Envia para framebuffer (tela)
    fb_write(prefix, 8);
    fb_write((char*)msg, len);
    fb_write(newline, 1);
    
    // Envia para serial (arquivo de log)
    serial_write(SERIAL_COM1_BASE, prefix, 8);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, newline, 1);
}
