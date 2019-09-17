/*
 * Copyright (C) 2018 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
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

typedef void (*usbus_cdcacm_cb_t)(usbus_t *usbus, usbus_cdcacm_device_t *cdc,
                                  uint8_t *data, size_t len);

struct usbus_cdcacm_device {
    usbus_handler_t handler_ctrl;
    usbus_interface_t iface_ctrl;
    usbus_interface_t iface_data;
    usbus_hdr_gen_t cdcacm_hdr;
    usbus_cdcacm_cb_t cb;
    tsrb_t tsrb;
    usbus_t *usbus;
    size_t occupied;
    usbus_cdcacm_line_state_t state;
    event_t flush;
};

int cdc_init(usbus_t *usbus, usbus_cdcacm_device_t *handler,
             usbus_cdcacm_cb_t cb, uint8_t *buf, size_t len);
size_t usbus_cdc_acm_submit(usbus_cdcacm_device_t *cdcacm, const char *buf, size_t len);
#ifdef __cplusplus
}
#endif

#endif /* USB_USBUS_CDC_ACM_H */
/** @} */
