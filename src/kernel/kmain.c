#include "fb.h"
#include "serial.h"

void kmain(void)
{
    char *welcome = "OS Iniciado!";
    char *serial_msg = "[INFO] Sistema carregado\n";
    
    serial_init();
    fb_write(welcome, 12);
    serial_write(SERIAL_COM1_BASE, serial_msg, 26);
    
    while(1);
}
