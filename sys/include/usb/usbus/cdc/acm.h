/*
 * Copyright (C) 2018 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @defgroup    usbus_cdc_acm USBUS CDC ACM - USBUS CDC abstract control model
 * @ingroup     usb
 * @brief       USBUS CDC ACM interface module
 *
 * @{
 *
 * @file
 * @brief       Interface and definitions for USB CDC ACM type interfaces in
 *              USBUS.
 *
 *              The functionality provided here only implements the USB
 *              specific handling. A different module is required to provide
 *              functional handling of the data e.g. UART or STDIO integration.
 *
 * @author      Dylan Laduranty <dylan.laduranty@mesotic.com>
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef USB_USBUS_CDC_ACM_H
#define USB_USBUS_CDC_ACM_H

#include <stdint.h>
#include "usb/usbus.h"
#include "tsrb.h"

#ifdef __cplusplus
extern "c" {
#endif

/**
 * @brief This flag configures the cdc acm to buffer stdout until the host
 *        signals that a listener is present
 */
#ifndef USBUS_CDCACM_BUFFER_FOR_DTE
#define USBUS_CDCACM_BUFFER_FOR_DTE  (1)
#endif /* USBUS_CDCACM_BUFFER_FOR_DTE */

/**
 * @brief Buffer size for STDIN and STDOUT data to and from USB
 */
#ifndef USBUS_CDC_ACM_STDIO_BUF_SIZE
#define USBUS_CDC_ACM_STDIO_BUF_SIZE 128
#endif

/**
 * @brief CDC ACM line state as reported by the host computer
 */
typedef enum {
    /**
     * @brief No DTE connected
     */
    USBUS_CDCACM_LINE_STATE_DISCONNECTED,

    /**
     * @brief DTE (e.g. a personal computer) is present and connected
     */
    USBUS_CDCACM_LINE_STATE_DTE
} usbus_cdcacm_line_state_t;

typedef struct usbus_cdcacm_device usbus_cdcacm_device_t;

/**
 * @brief CDC ACM data callback.
 *
 * Callback for received data from the USB host
 *
 * @param[in]   cdcacm  CDC ACM handler context
 * @param[in]   data    ptr to the data
 * @param[in]   len     Length of the received data
 */
typedef void (*usbus_cdcacm_cb_t)(usbus_cdcacm_device_t *cdcacm,
                                  uint8_t *data, size_t len);

/**
 * @brief CDC ACM line coding callback.
 *
 * Callback for received line coding request from the USB host
 *
 * @param[in]   cdcacm  CDC ACM handler context
 * @param[in]   baud    requested baud rate
 * @param[in]   bits    requested number of data bits
 * @param[in]   parity  requested parity
 * @param[in]   stop    requested number of stop bits
 *
 * @return              0 when the mode is available
 * @return              negative if the mode is not available
 */
typedef int (*usbus_cdcacm_coding_cb_t)(usbus_cdcacm_device_t *cdcacm,
                                        uint32_t baud, uint8_t bits,
                                        uint8_t parity, uint8_t stop);

struct usbus_cdcacm_device {
    usbus_handler_t handler_ctrl;
    usbus_interface_t iface_ctrl;
    usbus_interface_t iface_data;
    usbus_hdr_gen_t cdcacm_hdr;
    usbus_cdcacm_cb_t cb;
    usbus_cdcacm_coding_cb_t coding_cb;
    tsrb_t tsrb;
    usbus_t *usbus;
    size_t occupied;
    usbus_cdcacm_line_state_t state;
    event_t flush;
};

/**
 * @brief Initialize an USBUS CDC ACM interface
 *
 * @param[in]   usbus       USBUS context to register with
 * @param[in]   handler     USBUS CDC ACM handler
 * @param[in]   cb          Callback for data from the USB interface
 * @param[in]   coding_cb   Callback for control settings
 * @param[in]   buf         Buffer for data to the USB interface
 * @param[in]   len         Size in bytes of the buffer
 */
void usbus_cdc_acm_init(usbus_t *usbus, usbus_cdcacm_device_t *cdcacm,
                        usbus_cdcacm_cb_t cb,
                        usbus_cdcacm_coding_cb_t coding_cb,
                        uint8_t *buf, size_t len);

/**
 * @brief Submit bytes to the CDC ACM handler
 *
 * @param[in]   cdcacm      USBUS CDC ACM handler context
 * @param[in]   buf         buffer to submit
 * @param[in]   len         length of the submitted buffer
 *
 * @return                  Number of bytes added to the CDC ACM ring buffer
 */
size_t usbus_cdc_acm_submit(usbus_cdcacm_device_t *cdcacm,
                           const uint8_t *buf, size_t len);

/**
 * @brief Flush the buffer to the USB host
 *
 * @param[in]   cdcacm      USBUS CDC ACM handler context
 */
void usbus_cdc_acm_flush(usbus_cdcacm_device_t *cdcacm);

/**
 * @brief Set the callback for control settings
 *
 * Interrupts are disabled during update to ensure thread safety
 *
 * @param[in]   cdcacm      USBUS CDC ACM handler context
 * @param[in]   coding_cb   Callback for control settings
 */
void usbus_cdc_acm_set_coding_cb(usbus_cdcacm_device_t *cdcacm,
                                 usbus_cdcacm_coding_cb_t coding_cb);

#ifdef __cplusplus
}
#endif

#endif /* USB_USBUS_CDC_ACM_H */
/** @} */
