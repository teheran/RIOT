/*
 * Copyright (C) 2018 Dylan Laduranty
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup usb_acm Virtual Serial Port
 * @{
 * @file
 *
 * @author  Dylan Laduranty <dylan.laduranty@mesotic.com>
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include "tsrb.h"

#include "usb/descriptor.h"
#include "usb/cdc.h"
#include "usb/cdc/acm.h"
#include "usb/descriptor.h"
#include "usb/usbus.h"
#include "usb/usbus/cdc/acm.h"
#include "usb/usbus/control.h"

#include <string.h>

#define ENABLE_DEBUG    (0)
#include "debug.h"

static void _init(usbus_t *usbus, usbus_handler_t *handler);
static void _event_handler(usbus_t *usbus, usbus_handler_t *handler, usbus_event_usb_t event);
static int _setup_handler(usbus_t *usbus, usbus_handler_t *handler,
                          usbus_setuprq_state_t state, usb_setup_t *setup);
static void _transfer_handler(usbus_t *usbus, usbus_handler_t *handler,
                             usbdev_ep_t *ep, usbus_event_transfer_t event);

static void _cdc_acm_flush(usbus_cdcacm_device_t *cdcacm);
static void _handle_flush(event_t *ev);

static const usbus_handler_driver_t cdc_driver = {
    .init = _init,
    .event_handler = _event_handler,
    .setup_handler = _setup_handler,
    .transfer_handler = _transfer_handler,
};

static size_t _gen_full_acm_descriptor(usbus_t *usbus, void *arg);

static const usbus_hdr_gen_funcs_t _cdcacm_descriptor = {
    .get_header = _gen_full_acm_descriptor,
    .len = {
        .fixed_len = sizeof(usb_desc_cdc_t) +
                     sizeof(usb_desc_acm_t) +
                     sizeof(usb_desc_union_t) +
                     sizeof(usb_desc_call_mngt_t),
    },
    .len_type = USBUS_HDR_LEN_FIXED,
};

size_t usbus_cdc_acm_submit(usbus_cdcacm_device_t *cdcacm, const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        tsrb_add_one(&cdcacm->tsrb, buf[i]);
        if (buf[i] == '\n') {
            _cdc_acm_flush(cdcacm);
        }
    }
    return len;
}

/* Signal flush */
void _cdc_acm_flush(usbus_cdcacm_device_t *cdcacm)
{
    usbus_event_post(cdcacm->usbus, &cdcacm->flush);
}

static size_t _gen_mngt_descriptor(usbus_t *usbus)
{
    usb_desc_call_mngt_t mngt;
    /* functional call management descriptor */
    mngt.length = sizeof(usb_desc_call_mngt_t);
    mngt.type = USB_TYPE_DESCRIPTOR_CDC;
    mngt.subtype = 0x1;
    mngt.capabalities = 0;
    mngt.data_if = 1;
    usbus_control_slicer_put_bytes(usbus, (uint8_t*)&mngt, sizeof(mngt));
    return sizeof(usb_desc_call_mngt_t);
}

static size_t _gen_union_descriptor(usbus_t *usbus,
                                    usbus_cdcacm_device_t *cdcacm)
{
    usb_desc_union_t uni;
    /* functional union descriptor */
    uni.length = sizeof(usb_desc_union_t);
    uni.type = USB_TYPE_DESCRIPTOR_CDC;
    uni.subtype = 0x6;
    uni.master_if = cdcacm->iface_ctrl.idx;
    uni.slave_if = cdcacm->iface_data.idx;
    usbus_control_slicer_put_bytes(usbus, (uint8_t*)&uni, sizeof(uni));
    return sizeof(usb_desc_union_t);
}

static size_t _gen_acm_descriptor(usbus_t *usbus)
{
    usb_desc_acm_t acm;
    /* functional cdc acm descriptor */
    acm.length = sizeof(usb_desc_acm_t);
    acm.type = USB_TYPE_DESCRIPTOR_CDC;
    acm.subtype = 0x02;
    acm.capabalities = 0x00;
    usbus_control_slicer_put_bytes(usbus, (uint8_t*)&acm, sizeof(acm));
    return sizeof(usb_desc_acm_t);
}

static size_t _gen_cdc_descriptor(usbus_t *usbus)
{
    usb_desc_cdc_t cdc;
    /* functional cdc descriptor */
    cdc.length = sizeof(usb_desc_cdc_t);
    cdc.bcd_cdc = 0x0120;
    cdc.type = USB_TYPE_DESCRIPTOR_CDC;
    cdc.subtype = 0x00;
    usbus_control_slicer_put_bytes(usbus, (uint8_t*)&cdc, sizeof(cdc));
    return sizeof(usb_desc_cdc_t);
}

static size_t _gen_full_acm_descriptor(usbus_t *usbus, void *arg)
{
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)arg;
    size_t total_len = 0;
    total_len += _gen_cdc_descriptor(usbus);
    total_len += _gen_acm_descriptor(usbus);
    total_len += _gen_union_descriptor(usbus, cdcacm);
    total_len += _gen_mngt_descriptor(usbus);
    return total_len;
}

int cdc_init(usbus_t *usbus, usbus_cdcacm_device_t *handler,
             usbus_cdcacm_cb_t cb, uint8_t *buf, size_t len)
{
    memset(handler, 0 , sizeof(usbus_cdcacm_device_t));
    handler->usbus = usbus;
    tsrb_init(&handler->tsrb, buf, len);
    handler->handler_ctrl.driver = &cdc_driver;
    handler->cb = cb;
    handler->state = USBUS_CDCACM_LINE_STATE_DISCONNECTED;
    usbus_register_event_handler(usbus, (usbus_handler_t*)handler);
    return 0;
}

static void _init(usbus_t *usbus, usbus_handler_t *handler)
{
    DEBUG("CDC_ACM: intialization\n");
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)handler;

    cdcacm->flush.handler = _handle_flush;

    cdcacm->cdcacm_hdr.next = NULL;
    cdcacm->cdcacm_hdr.funcs = &_cdcacm_descriptor;
    cdcacm->cdcacm_hdr.arg = cdcacm;

    /* Instantiate interfaces */
    memset(&cdcacm->iface_ctrl, 0, sizeof(usbus_interface_t));
    memset(&cdcacm->iface_data, 0, sizeof(usbus_interface_t));
    /* Configure Interface 0 as control interface */
    cdcacm->iface_ctrl.class = USB_CLASS_CDC_CONTROL ;
    cdcacm->iface_ctrl.subclass = USB_CDC_SUBCLASS_ACM;
    cdcacm->iface_ctrl.protocol = USB_CDC_PROTOCOL_NONE;
    cdcacm->iface_ctrl.hdr_gen = &cdcacm->cdcacm_hdr;
    cdcacm->iface_ctrl.handler = handler;
    /* Configure second interface to handle data endpoint */
    cdcacm->iface_data.class = USB_CLASS_CDC_DATA ;
    cdcacm->iface_data.subclass = USB_CDC_SUBCLASS_NONE;
    cdcacm->iface_data.protocol = USB_CDC_PROTOCOL_NONE;
    cdcacm->iface_data.hdr_gen = NULL;
    cdcacm->iface_data.handler = handler;

    /* Create required endpoints */
    cdcacm->ep_ctrl = usbus_add_endpoint(usbus, &cdcacm->iface_ctrl, USB_EP_TYPE_INTERRUPT, USB_EP_DIR_IN, 8);
    cdcacm->ep_ctrl->interval = 255;
    cdcacm->ep_data_out = usbus_add_endpoint(usbus, &cdcacm->iface_data, USB_EP_TYPE_BULK, USB_EP_DIR_OUT, 64);
    cdcacm->ep_data_out->interval = 0;
    cdcacm->ep_data_in = usbus_add_endpoint(usbus, &cdcacm->iface_data, USB_EP_TYPE_BULK, USB_EP_DIR_IN, 64);
    cdcacm->ep_data_in->interval = 0;
    /* Add interfaces to the stack */
    usbus_add_interface(usbus, &cdcacm->iface_ctrl);
    usbus_add_interface(usbus, &cdcacm->iface_data);

    usbdev_ep_ready(cdcacm->ep_data_out->ep, 0);
    usbus_enable_endpoint(cdcacm->ep_data_out);
    usbus_enable_endpoint(cdcacm->ep_data_in);
    usbus_enable_endpoint(cdcacm->ep_ctrl);
    usbus_handler_set_flag(handler, USBUS_HANDLER_FLAG_RESET);
}

static int _setup_handler(usbus_t *usbus, usbus_handler_t *handler,
                          usbus_setuprq_state_t state, usb_setup_t *setup)
{
    (void)state;
    (void)usbus;
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)handler;
    DEBUG("Request:0x%x\n", setup->request);
    switch(setup->request) {
        case USB_SETUP_REQ_SET_LINE_CODING:
            DEBUG("Value:0x%x, interface:%d, len:%d\n", setup->value,
                                                        setup->index,
                                                        setup->length);
            break;
        case USB_SETUP_REQ_SET_CONTROL_LINE_STATE:
            if (setup->value & USB_CDC_ACM_CONTROL_LINE_DTE) {
                DEBUG("CDC ACM: DTE Enabled\n");
                cdcacm->state = USBUS_CDCACM_LINE_STATE_DTE;
                _cdc_acm_flush(cdcacm);
            }
            else {
                cdcacm->state = USBUS_CDCACM_LINE_STATE_DISCONNECTED;
                DEBUG("CDC ACM: DTE disabled\n");
            }
            DEBUG("Value:0x%x, interface:%d, nb:%d\n", setup->value,
                                                       setup->index,
                                                       setup->length);
            break;
        default:
            DEBUG("default handle setup rqt:0x%x\n", setup->request);
            break;
    }
    return 0;
}

static void _handle_in(usbus_t *usbus, usbus_handler_t *handler)
{
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)handler;
    if (usbus->state != USBUS_STATE_CONFIGURED ||
            cdcacm->state != USBUS_CDCACM_LINE_STATE_DTE) {
        return;
    }
    usbdev_ep_t *ep = cdcacm->ep_data_in->ep;
    while (!tsrb_empty(&cdcacm->tsrb)) {
        int c = tsrb_get_one(&cdcacm->tsrb);
        ep->buf[cdcacm->occupied++] = (uint8_t)c;
        if (cdcacm->occupied >= 64) {
            break;
        }
    }
    usbdev_ep_ready(ep, cdcacm->occupied);
}

static void _transfer_handler(usbus_t *usbus, usbus_handler_t *handler,
                             usbdev_ep_t *ep, usbus_event_transfer_t event)
{
    (void)event; /* Only receives TR_COMPLETE events */
    usbus_cdcacm_device_t *cdc = (usbus_cdcacm_device_t*)handler;
    if (ep == cdc->ep_data_out->ep) {
        size_t len;
        /* Retrieve incoming data */
        usbdev_ep_get(ep, USBOPT_EP_AVAILABLE, &len, sizeof(size_t));
        if (len > 0) {
            cdc->cb(usbus, cdc, ep->buf, len);
        }
        usbdev_ep_ready(ep, 0);
    }
    else if (ep == cdc->ep_data_in->ep) {
        cdc->occupied = 0;
        if (!tsrb_empty(&cdc->tsrb)) {
            return _handle_in(usbus, handler);
        }
    }
}

static void _handle_flush(event_t *ev)
{
    usbus_cdcacm_device_t *cdcacm = container_of(ev, usbus_cdcacm_device_t,
                                                 flush);
    if (cdcacm->occupied == 0) {
        _handle_in(cdcacm->usbus, (usbus_handler_t*)cdcacm);
    }
}

static void _handle_reset(usbus_handler_t *handler)
{
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t *)handler;
    DEBUG("CDC ACM: Reset\n");

    cdcacm->state = USBUS_CDCACM_LINE_STATE_DISCONNECTED;
}

static void _event_handler(usbus_t *usbus, usbus_handler_t *handler, usbus_event_usb_t event)
{
    (void)usbus;
    switch(event) {
        case USBUS_EVENT_USB_RESET:
            _handle_reset(handler);
            break;

        default:
            DEBUG("Unhandled event :0x%x\n", event);
            break;
    }
}
