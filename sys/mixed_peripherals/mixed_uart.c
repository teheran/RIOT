#include <periph/uart.h>

/** @FIXME these should rather come from frobnicated periph/uart.h */
int efm32_uart_init(efm32_uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg);
void efm32_uart_write(efm32_uart_t uart, const uint8_t *data, size_t len);

#include <pipe_uart.h>
#include <semihosted_uart.h>

/* FIXME switch everything to X maros */

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
	switch(uart.cls)
	{
	case class_efm32: return efm32_uart_init(uart.arg.efm32, baudrate, rx_cb, arg);
	case class_pipe: return pipe_uart_init(uart.arg.pipe, baudrate, rx_cb, arg);
	case class_semihosted: return semihosted_uart_init(uart.arg.semihosted, baudrate, rx_cb, arg);
	}
	return -1;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
	switch(uart.cls)
	{
	case class_efm32: efm32_uart_write(uart.arg.efm32, data, len); return;
	case class_pipe: pipe_uart_write(uart.arg.pipe, data, len); return;
	case class_semihosted: semihosted_uart_write(uart.arg.semihosted, data, len); return;
	}
}
