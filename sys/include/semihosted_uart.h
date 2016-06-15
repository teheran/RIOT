#ifndef SEMIHOSTED_UART_H
#define SEMIHOSTED_UART_H

typedef struct {
	int slot;
	const char *infile; /** @FIXME it's superfluous that this is passed to _write as well, but with current uart_init semantics, everything that goes to uart_init must go to uart__write too */
	const char *outfile;
} semihosted_uart_t;

/* not on top because periph/uart will include this back and need semihosted_uart_t */
#include <periph/uart.h>

/** @FIXME copied over from periph/uart.h before messing with typedef sequence there */
typedef void(*uart_rx_cb_t)(void *arg, uint8_t data);

int semihosted_uart_init(semihosted_uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg);
void semihosted_uart_write(semihosted_uart_t uart, const uint8_t *data, size_t len);

/** Call this if you want to have a thread that keeps polling for new reads and calls the appropriate callbacks */
void semihosted_uart_startthread(void);

#endif
