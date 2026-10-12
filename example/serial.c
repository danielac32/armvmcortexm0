#include <stdint.h>

/* UART at 0xE0000000 - writes produce simulator console output */
#define UART_DR (*(volatile uint32_t *)0xE0000000)

void serial_init(void)
{
    /* Nothing to init - UART is emulated */
}

void serial_putchar(char ch)
{
    if (ch == '\n')
        serial_putchar('\r');
    UART_DR = (uint32_t)ch;
}

void serial_putstr(const char *str)
{
    while (*str)
        serial_putchar(*str++);
}
