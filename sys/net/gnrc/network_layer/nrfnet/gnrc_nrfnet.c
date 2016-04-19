/*
 * Copyright (C) 2016 Bas Stottelaar <basstottelaar@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <errno.h>

#include "byteorder.h"
#include "thread.h"
#include "msg.h"
#include "net/gnrc/nrfnet.h"
#include "net/gnrc.h"

#include "xtimer.h"
#include "timex.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

/**
 * @brief   Stack for the nrfnet thread
 */
#if ENABLE_DEBUG
static char _stack[GNRC_NRFNET_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_NRFNET_STACK_SIZE];
#endif

/**
 * @brief   Reassembly buffer memory allocation.
 * @{
 */
#ifdef MODULE_GNRC_NRFNET_FRAG
static nrfnet_rbuf_t _rbuf;
static nrfnet_head_t _rbuf_heads[GNRC_NRFNET_FRAG_HEADS]
static nrfnet_hole_t _rbuf_holes[GNRC_NRFNET_FRAG_HOLES];
#endif
/** @} */

/**
 * @brief   PID of the nrfnet thread
 */
kernel_pid_t gnrc_nrfnet_pid = KERNEL_PID_UNDEF;

/**
 * @brief   Handle data to send.
 */
static void _send(gnrc_pktsnip_t* pkt)
{
    (void) pkt;
}

/**
 * @brief   Handle data received from an nRF24L01+ transceiver.
 */
static void _recv(gnrc_pktsnip_t* pkt)
{
    kernel_pid_t iface = KERNEL_PID_UNDEF;
    gnrc_pktsnip_t *nrfnet, *netif;
    nrfnet_hdr_t hdr;

#ifdef MODULE_GNRC_NRFNET_FRAG
    nrfnet_rbuf_head_t *head;
#endif
#ifdef MODULE_GNRC_NRFNET_ROUTER
    gnrc_pktnsip_t *pkt_reversed;
#endif

    /* assert that there is a packet */
    assert(pkt != NULL);

    /* validate packet size */
    if (pkt->size < sizeof(nrfnet_comp_hdr_t)) {
        DEBUG("gnrc_nrfnet: packet smaller than header\n");
        gnrc_pktbuf_release(pkt);
        return;
    }

    if (pkt->size > NRFNET_PAYLOAD_SIZE) {
        DEBUG("gnrc_nrfnet: packet size too large\n");
        gnrc_pktbuf_release(pkt);
        return;
    }

    /* uncompress the header and check if valid */
    nrfnet_hdr_uncompress(&hdr, (nrfnet_comp_hdr_t*) pkt->data);

    if (!nrfnet_hdr_is_valid(&hdr)) {
        DEBUG("gnrc_nrfnet: received invalid header\n");
        gnrc_pktbuf_release(pkt);
        return;
    }

    /* forward packet if this is not the destination */
#ifdef MODULE_GNRC_NRFNET_ROUTER
    if (!nrfnet_addr_equal(hdr->to, NULL)) {
        DEBUG("gnrc_nrfnet: forwarding packet");
        _send(pkt_reversed);
        return;
    }
#else
    if (!nrfnet_addr_equal(hdr->to, NULL)) {
        DEBUG("gnrc_nrfnet: packet routing not supported\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
#endif

    /* handle fragmented packets (or not) */
#ifdef MODULE_GNRC_NRFNET_FRAG
    nrfnet_rbuf_gc(&_rbuf, xtimer_now() / MS_IN_USEC, GNRC_NRFNET_FRAG_EXPIRE);

    if (nrfnet_hdr_is_fragmented(&hdr)) {
        head = nrfnet_rbuf_add(&_rbuf, &hdr, &pkt->data[sizeof(nrfnet_comp_hdr_t)], pkt->size - sizeof(nrfnet_comp_hdr_t), xtimer_now() / MS_IN_USEC);

        if (head == NULL) {
            DEBUG("gnrc_nrfnet: unable to queue fragment\n");
            gnrc_pktbuf_release(pkt);
            return;
        }

        /* free old packet */
        gnrc_pktbuf_release(pkt);

        /* wait for the other fragments before continueing */
        if (head->fragments > 0) {
            return;
        }

        /* packet is ready, copy to a new buffer */
        pkt = gnrc_pktbuf_add(hdr, NULL, head->hdr->length, GNRC_NETTYPE_UNDEF);

        if (pkt == NULL) {
            DEBUG("gnrc_nrfnet: unable to allocate full packet\n");
            nrfnet_rbuf_free(head);
            return;
        }

        if (nrfnet_rbuf_cpy(head, pkt->data, pkt->size) == NULL) {
            DEBUG("gnrc_nrfnet: unable to copy from fragment buffer\n");
            nrfnet_rbuf_free(head);
        }

        /* make room in fragment buffer */
        nrfnet_rbuf_free(head);
    }
#else
    if (nrfnet_hdr_is_fragmented(&hdr)) {
        DEBUG("gnrc_nrfnet: fragmented packets not supported\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
#endif

    /* forward packet to next layer */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_NRFNET, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        DEBUG("gnrc_nrfnet: no thread is interested in packet\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
}

static void *_eventloop(void *arg)
{
    (void)arg;
    msg_t msg, reply;
    msg_t msg_queue[GNRC_NRFNET_MSG_QUEUE_SIZE];

    /* setup the message queue */
    msg_init_queue(msg_queue, GNRC_NRFNET_MSG_QUEUE_SIZE);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                _recv(&state, (gnrc_pktsnip_t *)msg.content.ptr);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;
            default:
                break;
        }
    }

    /* never reached */
    return NULL;
}

kernel_pid_t gnrc_nrfnet_init(void)
{
    if (gnrc_nrfnet_pid == KERNEL_PID_UNDEF) {
        gnrc_nrfnet_pid = thread_create(_stack, sizeof(_stack),
                                        GNRC_NRFNET_PRIO,
                                        THREAD_CREATE_STACKTEST,
                                        _eventloop, NULL, "nrfnet");
    }

    /* configure reassembly buffer */
#ifdef MODULE_GNRC_NRFNET_FRAG
    _rbuf.heads = _rbuf_heads;
    _rbuf.holes = _rbuf_holes;
    _rbuf.head_count = GNRC_NRFNET_FRAG_HEADS;
    _rbuf.hole_count = GNRC_NRFNET_FRAG_HOLES;

    nrfnet_rbuf_init(&_rbuf);
#endif

    return gnrc_nrfnet_pid;
}
