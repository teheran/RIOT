/*
 * Copyright (C) 2018 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup sys
 * @{
 *
 * @file
 * @brief CDC ACM stdio implementation
 *
 * This file implements a USB CDC ACM callback and read/write functions.
 *
 *
 * @}
 */

#include <stdio.h>
#if MODULE_VFS
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif
#include "stdio_uart.h"

#include "board.h"
#include "isrpipe.h"

#include "usb/usbus.h"
#include "usb/usbus/cdc/acm.h"

#if MODULE_VFS
#include "vfs.h"
#endif

static usbus_cdcacm_device_t cdcacm;
static uint8_t _cdc_tx_buf_mem[STDIO_UART_RX_BUFSIZE * 4];
static uint8_t _cdc_rx_buf_mem[STDIO_UART_RX_BUFSIZE];
isrpipe_t cdc_stdio_isrpipe = ISRPIPE_INIT(_cdc_rx_buf_mem);

#if MODULE_VFS
static ssize_t uart_stdio_vfs_read(vfs_file_t *filp, void *dest, size_t nbytes);
static ssize_t uart_stdio_vfs_write(vfs_file_t *filp, const void *src, size_t nbytes);

/**
 * @brief VFS file operation table for stdin/stdout/stderr
 */
static vfs_file_ops_t uart_stdio_vfs_ops = {
    .read = stdio_vfs_read,
    .write = stdio_vfs_write,
};

static ssize_t stdio_vfs_read(vfs_file_t *filp, void *dest, size_t nbytes)
{
    int fd = filp->private_data.value;
    if (fd != STDIN_FILENO) {
        return -EBADF;
    }
    return uart_stdio_read(dest, nbytes);
}

static ssize_t stdio_vfs_write(vfs_file_t *filp, const void *src, size_t nbytes)
{
    int fd = filp->private_data.value;
    if (fd == STDIN_FILENO) {
        return -EBADF;
    }
    return uart_stdio_write(src, nbytes);
}
#endif

void stdio_init(void)
{
    /* Initialize this side of the CDC ACM pipe */
#if MODULE_VFS
    int fd;
    fd = vfs_bind(STDIN_FILENO, O_RDONLY, &uart_stdio_vfs_ops, (void *)STDIN_FILENO);
    if (fd < 0) {
        /* How to handle errors on init? */
    }
    fd = vfs_bind(STDOUT_FILENO, O_WRONLY, &uart_stdio_vfs_ops, (void *)STDOUT_FILENO);
    if (fd < 0) {
        /* How to handle errors on init? */
    }
    fd = vfs_bind(STDERR_FILENO, O_WRONLY, &uart_stdio_vfs_ops, (void *)STDERR_FILENO);
    if (fd < 0) {
        /* How to handle errors on init? */
    }
#endif
}

ssize_t stdio_read(void* buffer, size_t len)
{
    (void)buffer;
    (void)len;
    return isrpipe_read(&cdc_stdio_isrpipe, buffer, len);
}

ssize_t stdio_write(const void* buffer, size_t len)
{
    usbus_cdc_acm_submit(&cdcacm, buffer, len);
    /* Use tsrb and flush */
    return len;
}

void cdcacm_rx_pipe(usbus_t *usbus, usbus_cdcacm_device_t *cdcacm,
                           uint8_t *data, size_t len)
{
    (void)usbus;
    (void)cdcacm;
    for (size_t i = 0; i < len; i++) {
        isrpipe_write_one(&cdc_stdio_isrpipe, data[i]);
    }
}

void usb_cdcacm_stdio_init(usbus_t *usbus)
{
    cdc_init(usbus, &cdcacm, cdcacm_rx_pipe, _cdc_tx_buf_mem,
             sizeof(_cdc_tx_buf_mem));
}
