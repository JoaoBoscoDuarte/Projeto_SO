#ifndef INCLUDE_SERIAL_H
#define INCLUDE_SERIAL_H

#define SERIAL_COM1_BASE 0x3F8

void serial_configure_baud_rate(unsigned short com, unsigned short divisor);
void serial_configure_line(unsigned short com);
void serial_configure_buffers(unsigned short com);
void serial_configure_modem(unsigned short com);
int serial_is_transmit_fifo_empty(unsigned int com);
void serial_write(unsigned short com, char *buf, unsigned int len);
void serial_init(void);

#endif