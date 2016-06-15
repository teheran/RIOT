/** UART implementation that uses (almost) the established mechanisms of ARM
 * semihosting.
 *
 * The only adaption this needs to be practical on the side of the host is that
 * a SYS_READ return EOF (r0 = full requeted length) if no data is pending
 * instead of blocking the whole MCU.
 *
 * In general, I'm not particularly convinced that the way semihosting is
 * implemented here is a *good* way of doing things, because
 *
 * a. every write and read causes the MCU to halt completely until the host
 *   decides to answer (instead of placing the request and being woken by an
 *   interrupt when a response is there, which would enable RIOT threads to do
 *   work inbetween), and
 *
 * b. reads have to be polled actively instead of issuing an interrupt in some
 *   kind of select(2)-style mechanism. (That wouldn't be too bad if a. was
 *   solved, but still it would be better if we wouldn't need an input thread
 *   but could just configure some interrupt to arrive and then decide which
 *   callback the host wants us to execute).
 */

#include <semihosted_uart.h>
#include <string.h>
#include <xtimer.h>
#include <thread.h>

#define SEMIHOSTED_UART_NUM 3

static struct {
	int in_handle;
	int out_handle;
	uart_rx_cb_t rx_cb;
	void *arg;
} uarts[SEMIHOSTED_UART_NUM];

/** @FIXME this is very fragile and only works when not inlined; better
 * understanding of how GCC deals with register access would be greatly
 * appreciated */
static int __attribute__((noinline)) semihosted_syscall(int syscall, void *data)
{
	register int tmp asm ("r2");
	__asm__("mov r0, %0" : : "rim"(syscall));
	__asm__("mov r1, %0" : : "rim"(data));
	tmp = 1234;
	__asm__("bkpt #0xAB" : : : "r0");
	__asm__("mov %0, r0" : "=r"(tmp));
	return tmp;
}

int semihosted_uart_init(semihosted_uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
	struct request {
		const char *filename;
		uint32_t mode;
		uint32_t length;
	};

	if (uart.slot >= SEMIHOSTED_UART_NUM) return -1;

	struct request out_request = {uart.outfile, 5, strlen(uart.outfile)};
	uarts[uart.slot].out_handle = semihosted_syscall(0x01, &out_request);

	struct request in_request = {uart.infile, 1, strlen(uart.infile)};
	uarts[uart.slot].in_handle = semihosted_syscall(0x01, &in_request);

	uarts[uart.slot].rx_cb = rx_cb;
	uarts[uart.slot].arg = arg;

	return 0;
}

void semihosted_uart_write(semihosted_uart_t uart, const uint8_t *data, size_t len)
{
	if (uart.slot >= SEMIHOSTED_UART_NUM) return;
	struct {
		uint32_t handle;
		const uint8_t *data;
		uint32_t length;
	} write_request = {uarts[uart.slot].out_handle, data, len};

	semihosted_syscall(0x05, (void*)&write_request);
}

char semihosted_backcaller_stack[THREAD_STACKSIZE_DEFAULT];
static void *semihosted_backcaller(void *arg)
{
	int i = 0;
	uint8_t buffer[20]; /* We'll try to read more than one byte at a time; it's bad enough we regularly hit breakpoints, let's try at least to do some real work in one syscall */
	while (1) {
		xtimer_usleep(100000);
		if (uarts[i].rx_cb != NULL) {
			struct {
				int handle;
				uint8_t *buffer;
				int size;
			} read_request = {uarts[i].in_handle, buffer, sizeof(buffer)};
			int remaining = semihosted_syscall(0x06, &read_request);
			if (remaining >= 0 && remaining < sizeof(buffer)) {
				for (int j = 0; j < sizeof(buffer) - remaining; ++j) {
					uarts[i].rx_cb(uarts[i].arg, buffer[j]);
				}
			}
		}

		i = (i + 1) % SEMIHOSTED_UART_NUM;
	}

	UNREACHABLE();
}

void semihosted_uart_startthread(void)
{
	thread_create(semihosted_backcaller_stack, sizeof(semihosted_backcaller_stack),
			THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
			semihosted_backcaller, NULL, "semihosted_uart");
}
