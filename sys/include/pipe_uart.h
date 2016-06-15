#ifndef PIPE_UART_H
#define PIPE_UART_H

#include <pipe.h>

typedef struct {
	pipe_t *out;
	int in;
} pipe_uart_t;

/* not on top because periph/uart will include this back and need pipe_uart_t */
#include <periph/uart.h>

/** @FIXME copied over from periph/uart.h before messing with typedef sequence there */
typedef void(*uart_rx_cb_t)(void *arg, uint8_t data);

int pipe_uart_init(pipe_uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg);
void pipe_uart_write(pipe_uart_t uart, const uint8_t *data, size_t len);

/* this is to be called from the main thread; pipe_uart_init can be called
 * earlier (and will in case of the stdio uart!), but no callbacks will be
 * fired before this has launched its thread. */
void pipe_uart_globalinit(void);

#endif
