#ifndef NRFNET_RBUF_H
#define NRFNET_RBUF_H

#include <stdint.h>
#include <stdbool.h>

#include "net/nrfnet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Definition of a hole.
 */
typedef struct hole {
    bool used;

    uint8_t offset;
    uint8_t data[NRFNET_PAYLOAD_SIZE];

    struct hole* next;
} nrfnet_rbuf_hole_t;

/**
 * @brief   Definition of a head.
 */
typedef struct {
    bool used;

    nrfnet_hdr_t hdr;
    uint32_t timestamp;
    uint8_t fragments;

    nrfnet_rbuf_hole_t* next;
} nrfnet_rbuf_head_t;

/**
 * @brief   Definition of a reassembly buffer.
 */
typedef struct {
    nrfnet_rbuf_hole_t* holes;
    nrfnet_rbuf_head_t* heads;
    uint8_t hole_count;
    uint8_t head_count;
} nrfnet_rbuf_t;

void nrfnet_rbuf_init(nrfnet_rbuf_t* rbuf);
void nrfnet_rbuf_dump(nrfnet_rbuf_t* rbuf);
void nrfnet_rbuf_gc(nrfnet_rbuf_t* rbuf, uint32_t timestamp, uint32_t lifetime);
void nrfnet_rbuf_free(nrfnet_rbuf_head_t* head);
nrfnet_rbuf_head_t* nrfnet_rbuf_add(nrfnet_rbuf_t* rbuf, nrfnet_hdr_t* hdr, uint8_t* data, size_t length, uint32_t timestamp);
uint8_t* nrfnet_rbuf_cpy(nrfnet_rbuf_head_t* head, uint8_t* dest, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* NRFNET_H */
