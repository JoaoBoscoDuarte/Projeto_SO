#ifndef INCLUDE_PRINTF_H
#define INCLUDE_PRINTF_H

typedef enum {
    OUTPUT_FB,
    OUTPUT_SERIAL,
    OUTPUT_BOTH
} output_device_t;

void kprintf(output_device_t device, const char *fmt, ...);
void log_debug(const char *msg);
void log_info(const char *msg);
void log_error(const char *msg);

#endif
