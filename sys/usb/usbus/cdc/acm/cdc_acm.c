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

#include "msg.h"
#include "tsrb.h"

#include "usb/descriptor.h"
#include "usb/cdc.h"
#include "usb/cdc/acm.h"
#include "usb/usbus.h"
#include "usb/usbus/cdc/acm.h"

#include <string.h>

#define ENABLE_DEBUG    (0)
#include "debug.h"

static int event_handler(usbus_t *usbus, usbus_handler_t *handler, uint16_t event, void *arg);
static void _init(usbus_t *usbus, usbus_handler_t *handler);
void usbus_cdc_acm_flush(usbus_cdcacm_device_t *cdcacm);

static const usbus_handler_driver_t cdc_driver = {
    .init = _init,
    .event_handler = event_handler,
};

static size_t _gen_mngt_descriptor(usbus_t *usbus, void *arg);
static size_t _gen_mngt_size(usbus_t *usbus, void *arg);

static const usbus_hdr_gen_funcs_t _mngt_descriptor = {
    .get_header = _gen_mngt_descriptor,
    .get_header_len = _gen_mngt_size
};

static size_t _gen_union_descriptor(usbus_t *usbus, void *arg);
static size_t _gen_union_size(usbus_t *usbus, void *arg);

static const usbus_hdr_gen_funcs_t _union_descriptor = {
    .get_header = _gen_union_descriptor,
    .get_header_len = _gen_union_size
};

static size_t _gen_acm_descriptor(usbus_t *usbus, void *arg);
static size_t _gen_acm_size(usbus_t *usbus, void *arg);

static const usbus_hdr_gen_funcs_t _acm_descriptor = {
    .get_header = _gen_acm_descriptor,
    .get_header_len = _gen_acm_size
};

static size_t _gen_cdc_descriptor(usbus_t *usbus, void *arg);
static size_t _gen_cdc_size(usbus_t *usbus, void *arg);

static const usbus_hdr_gen_funcs_t _cdc_descriptor = {
    .get_header = _gen_cdc_descriptor,
    .get_header_len = _gen_cdc_size
};

size_t usbus_cdc_acm_submit(usbus_cdcacm_device_t *cdcacm, const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        tsrb_add_one(&cdcacm->tsrb, buf[i]);
        if (buf[i] == '\n') {
            usbus_cdc_acm_flush(cdcacm);
        }
    }
    return len;
}

/* Signal flush */
void usbus_cdc_acm_flush(usbus_cdcacm_device_t *cdcacm)
{
    msg_t msg = { .type = USBUS_MSG_CDCACM_FLUSH,
                 .content = { .ptr = cdcacm } };
    msg_send(&msg, cdcacm->usbus->pid);
}

static size_t _gen_mngt_descriptor(usbus_t *usbus, void *arg)
{
    (void)arg;
    usb_desc_call_mngt_t mngt;
    /* functional call management descriptor */
    mngt.length = sizeof(usb_desc_call_mngt_t);
    mngt.type = USB_TYPE_DESCRIPTOR_CDC;
    mngt.subtype = 0x1;
    mngt.capabalities = 0;
    mngt.data_if = 1;
    usbus_ctrlslicer_put_bytes(usbus, (uint8_t*)&mngt, sizeof(mngt));
    return sizeof(usb_desc_call_mngt_t);
}

static size_t _gen_mngt_size(usbus_t *usbus, void *arg)
{
    (void)usbus;
    (void)arg;
    return sizeof(usb_desc_call_mngt_t);
}

static size_t _gen_union_descriptor(usbus_t *usbus, void *arg)
{
    usbus_cdcacm_device_t *cdc = (usbus_cdcacm_device_t*)arg;
    usb_desc_union_t uni;
    /* functional union descriptor */
    uni.length = sizeof(usb_desc_union_t);
    uni.type = USB_TYPE_DESCRIPTOR_CDC;
    uni.subtype = 0x6;
    uni.master_if = cdc->iface_ctrl.idx;
    uni.slave_if = cdc->iface_data.idx;
    usbus_ctrlslicer_put_bytes(usbus, (uint8_t*)&uni, sizeof(uni));
    return sizeof(usb_desc_union_t);
}

static size_t _gen_union_size(usbus_t *usbus, void *arg)
{
    (void)usbus;
    (void)arg;
    return sizeof(usb_desc_union_t);
}

static size_t _gen_acm_descriptor(usbus_t *usbus, void *arg)
{
    (void)arg;
    usb_desc_acm_t acm;
    /* functional cdc acm descriptor */
    acm.length = sizeof(usb_desc_acm_t);
    acm.type = USB_TYPE_DESCRIPTOR_CDC;
    acm.subtype = 0x02;
    acm.capabalities = 0x00;
    usbus_ctrlslicer_put_bytes(usbus, (uint8_t*)&acm, sizeof(acm));
    return sizeof(usb_desc_acm_t);
}

static size_t _gen_acm_size(usbus_t *usbus, void *arg)
{
    (void)usbus;
    (void)arg;
    return sizeof(usb_desc_acm_t);
}

static size_t _gen_cdc_descriptor(usbus_t *usbus, void *arg)
{
    (void)arg;
    usb_desc_cdc_t cdc;
    /* functional cdc descriptor */
    cdc.length = sizeof(usb_desc_cdc_t);
    cdc.bcd_hid = 0x0120;
    cdc.type = USB_TYPE_DESCRIPTOR_CDC;
    cdc.subtype = 0x00;
    usbus_ctrlslicer_put_bytes(usbus, (uint8_t*)&cdc, sizeof(cdc));
    return sizeof(usb_desc_cdc_t);
}

static size_t _gen_cdc_size(usbus_t *usbus, void *arg)
{
    (void)usbus;
    (void)arg;
    return sizeof(usb_desc_cdc_t);
}

int cdc_init(usbus_t *usbus, usbus_cdcacm_device_t *handler,
             usbus_cdcacm_cb_t cb, char *buf, size_t len)
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
    usbus_cdcacm_device_t *cdc = (usbus_cdcacm_device_t*)handler;

    cdc->call_mngt_hdr.next = NULL;
    cdc->call_mngt_hdr.funcs = &_mngt_descriptor;
    cdc->call_mngt_hdr.arg = cdc;

    cdc->union_hdr.next = &cdc->call_mngt_hdr;
    cdc->union_hdr.funcs = &_union_descriptor;
    cdc->union_hdr.arg = cdc;

    cdc->acm_hdr.next = &cdc->union_hdr;
    cdc->acm_hdr.funcs = &_acm_descriptor;
    cdc->acm_hdr.arg = cdc;

    cdc->cdc_hdr.next = &cdc->acm_hdr;
    cdc->cdc_hdr.funcs = &_cdc_descriptor;
    cdc->cdc_hdr.arg = cdc;

    /* Instantiate interfaces */
    memset(&cdc->iface_ctrl, 0, sizeof(usbus_interface_t));
    memset(&cdc->iface_data, 0, sizeof(usbus_interface_t));
    /* Configure Interface 0 as control interface */
    cdc->iface_ctrl.class = USB_CLASS_CDC_CONTROL ;
    cdc->iface_ctrl.subclass = USB_CDC_SUBCLASS_ACM;
    cdc->iface_ctrl.protocol = USB_CDC_PROTOCOL_NONE;
    cdc->iface_ctrl.hdr_gen = &cdc->cdc_hdr;
    cdc->iface_ctrl.handler = handler;
    /* Configure second interface to handle data endpoint */
    cdc->iface_data.class = USB_CLASS_CDC_DATA ;
    cdc->iface_data.subclass = USB_CDC_SUBCLASS_NONE;
    cdc->iface_data.protocol = USB_CDC_PROTOCOL_NONE;
    cdc->iface_data.hdr_gen = NULL;
    cdc->iface_data.handler = handler;

    /* Create required endpoints */
    usbus_add_endpoint(usbus, &cdc->iface_ctrl, &cdc->ep_ctrl, USB_EP_TYPE_INTERRUPT, USB_EP_DIR_IN, 8);
    cdc->ep_ctrl.interval = 255;
    usbus_add_endpoint(usbus, &cdc->iface_data, &cdc->ep_data_out, USB_EP_TYPE_BULK, USB_EP_DIR_OUT, 64);
    cdc->ep_data_out.interval = 0;
    usbus_add_endpoint(usbus, &cdc->iface_data, &cdc->ep_data_in, USB_EP_TYPE_BULK, USB_EP_DIR_IN, 64);
    cdc->ep_data_in.interval = 0;
    /* Add interfaces to the stack */
    usbus_add_interface(usbus, &cdc->iface_ctrl);
    usbus_add_interface(usbus, &cdc->iface_data);

    //cdc->ep_data_in.ep->driver->ready(cdc->ep_data_in.ep, 0);
    usbdev_ep_ready(cdc->ep_data_out.ep, 0);
    //usbdev_ep_ready(cdc->ep_ctrl.ep, 0);
    usbus_enable_endpoint(&cdc->ep_data_out);
    usbus_enable_endpoint(&cdc->ep_data_in);
    usbus_enable_endpoint(&cdc->ep_ctrl);
}

static int _handle_setup(usbus_t *usbus, usbus_handler_t *handler, usb_setup_t *pkt)
{
    (void)usbus;
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)handler;
    DEBUG("Request:0x%x\n", pkt->request);
    switch(pkt->request) {
        case USB_SETUP_REQ_SET_LINE_CODING:
            DEBUG("Value:0x%x, interface:%d, len:%d\n",pkt->value, pkt->index, pkt->length);
            break;
        case USB_SETUP_REQ_SET_CONTROL_LINE_STATE:
            if (pkt->value & USB_CDC_ACM_CONTROL_LINE_DTE) {
                DEBUG("CDC ACM: DTE Enabled\n");
                cdcacm->state = USBUS_CDCACM_LINE_STATE_DTE;
                usbus_cdc_acm_flush(cdcacm);
            }
            else {
                cdcacm->state = USBUS_CDCACM_LINE_STATE_DISCONNECTED;
                DEBUG("CDC ACM: DTE disabled\n");
            }
            DEBUG("Value:0x%x, interface:%d, nb:%d\n",pkt->value, pkt->index, pkt->length);
            break;
        default:
            DEBUG("default handle setup rqt:0x%x\n", pkt->request);
            break;
    }
    return 0;
}

static int _handle_in(usbus_t *usbus, usbus_handler_t *handler)
{
    usbus_cdcacm_device_t *cdcacm = (usbus_cdcacm_device_t*)handler;
    if (usbus->state != USBUS_STATE_CONFIGURED || cdcacm->state != USBUS_CDCACM_LINE_STATE_DTE) {
        return -1;
    }
    usbdev_ep_t *ep = cdcacm->ep_data_in.ep;
    while (!tsrb_empty(&cdcacm->tsrb)) {
        char c = tsrb_get_one(&cdcacm->tsrb);
        ep->buf[cdcacm->occupied++] = (uint8_t)c;
        if (c == '\n' || cdcacm->occupied >= 64) {
            break;
        }
    }
    usbdev_ep_ready(ep, cdcacm->occupied);
    size_t len = cdcacm->occupied;
    cdcacm->occupied = 0;
    return len;
}

static int _handle_tr_complete(usbus_t *usbus, usbus_handler_t *handler, usbdev_ep_t *ep)
{
    usbus_cdcacm_device_t *cdc = (usbus_cdcacm_device_t*)handler;
    if (ep == cdc->ep_data_out.ep) {
        size_t len;
        /* Retrieve incoming data */
        usbdev_ep_get(ep, USBOPT_EP_AVAILABLE, &len, sizeof(size_t));
        if (len > 0) {
            cdc->cb(usbus, cdc, ep->buf, len);
        }
        usbdev_ep_ready(ep, 0);
        return 0;
    }
    else if (ep == cdc->ep_data_in.ep) {
        if (!tsrb_empty(&cdc->tsrb)) {
            return _handle_in(usbus, handler);
        }
    }
    return 0;
}

static int event_handler(usbus_t *usbus, usbus_handler_t *handler, uint16_t event, void *arg)
{
    switch(event) {
        case USBUS_MSG_TYPE_SETUP_RQ:
            return _handle_setup(usbus, handler, (usb_setup_t*)arg);
        case USBUS_MSG_TYPE_TR_COMPLETE:
            return _handle_tr_complete(usbus, handler, (usbdev_ep_t*)arg);
        case USBUS_MSG_CDCACM_FLUSH:
            return _handle_in(usbus, handler);
        default:
            DEBUG("Unhandled event :0x%x\n", event);
            return -1;
    }
    return 0;
}
