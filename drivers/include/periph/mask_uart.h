/** @file
 *
 * Multi-implementation diversion header
 *
 * In order to allow a UART implementation (typically cpu/.../periph/uart.c) to
 * be used simultaneously with other drivers, define a unique MASK_UART_PREFIX
 * and include this header before you include periph/uart.h.
 *
 * This is not strictly part of the uart class mechanism, but merely a helper
 * for unaware uart.c implementations to adopt it.
 *
 * */

#ifndef MASK_UART_PREFIX
#error "mask_uart.h can only be used following a MASK_UART_PREFIX "
#endif

#ifdef PERIPH_UART_H
#error "mask_uart.h must not be included after periph/uart.h" /* right now it's actually not a strict requirement and just provides declarations for the functions defined later; if the uart_class mechanisms were used, we'd need that strictness */
#endif

#define concat_helper(a, b) a ## b
#define concatenate(a, b) concat_helper(a, b)
#define apply_prefix(text) concatenate(MASK_UART_PREFIX, text)

#define uart_t apply_prefix(_uart_t)

#define uart_init apply_prefix(_uart_init)
#define uart_write apply_prefix(_uart_write)
#define uart_poweron apply_prefix(_uart_poweron)
#define uart_poweroff apply_prefix(_uart_poweroff)

/* we're working internally here, and thus the original uart_t default value should be used instead of the struct (which is for the unprefixe functions) */
#undef MIXED_PERIPHERALS
#include <periph/uart.h>
#define MIXED_PERIPHERALS

/* not needed yet (see comment around uart_class definition)
struct uart_class apply_prefix(_uart_class) = {
	.init = uart_init,
	.write = uart_write,
	.poweron = uart_poweron,
	.poweroff = uart_poweroff,
};
*/
