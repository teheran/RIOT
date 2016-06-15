#include <pipe_uart.h>
#include <xtimer.h>

#define NUM_PIPE_UARTS 1

static char data[NUM_PIPE_UARTS][200];
static ringbuffer_t buffer[NUM_PIPE_UARTS] = { RINGBUFFER_INIT(data[0]) };
static pipe_t inpipe[NUM_PIPE_UARTS];

struct pipecallback {
	uart_rx_cb_t rx_cb;
	void *arg;
};

static struct pipecallback rxcbs[NUM_PIPE_UARTS];

char pipe_backcaller_stack[THREAD_STACKSIZE_DEFAULT];
/** @FIXME several things:
 *
 * * this uses a sleep-poll-sleep cycle in order to be compatible with the
 *    crude asynchronous semihosting that can't yet create an interrupt on the
 *    mcu that would caue the chain of events leading to this thread that
 *    blocks on reading to wake up.
 *
 * * this executes the callbacks in a thread context instead of an interrupt
 *   context (but is that really bad?)
 *
 */
static void *pipe_backcaller(void *arg)
{
	int i = 0;
	while (1) {
		xtimer_usleep(100000);
		while (inpipe[i].rb->avail) {
			uint8_t buf[1];
			pipe_read(&inpipe[i], buf, 1);
			if (rxcbs[i].rx_cb != NULL)
				rxcbs[i].rx_cb(rxcbs[i].arg, buf[0]);
		}

		i = (i + 1) % NUM_PIPE_UARTS;
	}

	UNREACHABLE();
}

int pipe_uart_init(pipe_uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
	rxcbs[uart.in].rx_cb = rx_cb;
	rxcbs[uart.in].arg = arg;

	/* ignoring baud rate as customary with pipes. the alternative would be
	 * to go full telnet. */
	return 0;
}

void pipe_uart_write(pipe_uart_t uart, const uint8_t *data, size_t len)
{
	pipe_write(uart.out, data, len);
}

void pipe_uart_globalinit(void)
{
	for (int i = 0; i < NUM_PIPE_UARTS; ++i) {
		pipe_init(&inpipe[i], &buffer[i], NULL);
	}

	thread_create(pipe_backcaller_stack, sizeof(pipe_backcaller_stack),
			THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
			pipe_backcaller, NULL, "pipe_uartr");

	/* add the functions for gdb in myterm */
	ringbuffer_add_one(inpipe[0].rb, 'Z');
	ringbuffer_get_one(inpipe[0].rb);
}
