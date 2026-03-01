#include "printf.h"
#include "fb.h"
#include "serial.h"

static int my_strlen(const char *str)
{
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void itoa(int num, char *str)
{
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num != 0) {
        str[i++] = (num % 10) + '0';
        num = num / 10;
    }
    
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
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

static void itohex(unsigned int num, char *str)
{
    const char hex[] = "0123456789ABCDEF";
    int i = 0;
    
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    while (num != 0) {
        str[i++] = hex[num % 16];
        num = num / 16;
    }
    
    str[i] = '\0';
    
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

void kprintf(output_device_t device, const char *fmt, ...)
{
    char buffer[256];
    int buf_idx = 0;
    int *args = (int*)((char*)&fmt + sizeof(fmt));
    int arg_idx = 0;
    int i;
    
    for (i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i + 1] != '\0') {
            i++;
            if (fmt[i] == 's') {
                char *s = (char*)args[arg_idx++];
                while (*s) {
                    buffer[buf_idx++] = *s++;
                }
            } else if (fmt[i] == 'd') {
                char num_str[12];
                int j;
                itoa(args[arg_idx++], num_str);
                for (j = 0; num_str[j]; j++) {
                    buffer[buf_idx++] = num_str[j];
                }
            } else if (fmt[i] == 'x') {
                char hex_str[12];
                int j;
                itohex((unsigned int)args[arg_idx++], hex_str);
                for (j = 0; hex_str[j]; j++) {
                    buffer[buf_idx++] = hex_str[j];
                }
            } else {
                buffer[buf_idx++] = fmt[i];
            }
        } else {
            buffer[buf_idx++] = fmt[i];
        }
    }
    
    if (device == OUTPUT_FB || device == OUTPUT_BOTH) {
        fb_write(buffer, buf_idx);
    }
    if (device == OUTPUT_SERIAL || device == OUTPUT_BOTH) {
        serial_write(SERIAL_COM1_BASE, buffer, buf_idx);
    }
}

void log_debug(const char *msg)
{
    int len = my_strlen(msg);
    serial_write(SERIAL_COM1_BASE, "[DEBUG] ", 8);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, "\n", 1);
}

void log_info(const char *msg)
{
    int len = my_strlen(msg);
    char prefix[] = "[INFO] ";
    char newline[] = "\n";
    
    fb_write(prefix, 7);
    fb_write((char*)msg, len);
    fb_write(newline, 1);
    
    serial_write(SERIAL_COM1_BASE, prefix, 7);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, newline, 1);
}

void log_error(const char *msg)
{
    int len = my_strlen(msg);
    char prefix[] = "[ERROR] ";
    char newline[] = "\n";
    
    fb_write(prefix, 8);
    fb_write((char*)msg, len);
    fb_write(newline, 1);
    
    serial_write(SERIAL_COM1_BASE, prefix, 8);
    serial_write(SERIAL_COM1_BASE, (char*)msg, len);
    serial_write(SERIAL_COM1_BASE, newline, 1);
}