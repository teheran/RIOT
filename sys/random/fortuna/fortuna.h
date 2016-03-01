/**
 * @brief Fortuna PRNG implementation.
 *
 * The MIT License is applied to this software, see LICENSE.txt
 */

/*
 * This is not your general purpose PRNG, because it is hungry for memory.
 *
 * See https://www.schneier.com/cryptography/paperfiles/fortuna.pdf for the
 * implementation details. Code insipred by https://github.com/blin00/os (MIT
 * licensed), but heavily modified, stripped and improved for RIOT-OS.
 *
 * Still WIP because RIOT-OS does not have AES-256 (yet)!
 */

#ifndef FORTUNA_H
#define FORTUNA_H

#include "xtimer.h"

#include "crypto/aes.h"
#include "hashes/sha256.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @bief Number of pools to use, which may not exceed 32. More pools means more
 *       memory usage.
 */
#define FORTUNA_POOLS               (16)

/**
 * @brief Reseed timeout in us. Set to zero to disable.
 */
#define FORTUNA_RESEED_TIMEOUT      (100000U)

/**
 * @brief Generator counter and key.
 */
typedef struct {
    uint8_t key[32];
    union {
        struct {
            uint64_t l;
            uint64_t h;
        } split;
        uint8_t bytes[16];
    } counter;
} fortuna_generator_t;

/**
 * @brief Pool for storing entropy.
 */
typedef struct {
    sha256_context_t ctx;
    uint32_t len;
} fortuna_pool_t;

/**
 * @brief The fortuna state.
 */
typedef struct {
    fortuna_generator_t gen;
    fortuna_pool_t pools[FORTUNA_POOLS];
    uint32_t reseeds;
#if FORTUNA_RESEED_TIMEOUT > 0
    uint64_t last_reseed;
#endif
} fortuna_state_t;

/**
 * @brief   Initialize the Fortuna PRNG state.
 *
 * @param[in] state         PRNG state
 * @return                  zero on succesful initialization
 * @return                  non-zero on error
 */
int fortuna_init(fortuna_state_t *state);

/**
 * @brief   Read bytes from the PRNG. The number of bytes may not exceed 2^20
 *          bytes.
 *
 * @param[in] state         PRNG state
 * @param[out] out          pointer to buffer
 * @param[in] bytes         number of bytes to write in buffer
 * @return                  zero on success
 * @return                  -1 on reading from unseeded prng
 * @return                  -2 on reading more that 2^20 bytes
 */
int fortuna_read(fortuna_state_t *state, uint8_t *out, size_t bytes);

/**
 * @brief   Add entropy to one PRNG pool. The pool must exist and the source
 *          length must be 1-32 bytes.
 *
 * @param[in] state         PRNG state
 * @param[in] data          pointer to entropy source
 * @param[in] length        length of entropy source
 * @param[in] source        source identifier (each source has its own ID)
 * @param[in] pool          pool number to add entropy to
 * @return                  zero on success
 * @return                  -1 on zero bytes or more than 32 bytes
 * @return                  -2 on invalid pool number
 */
int fortuna_add_entropy(fortuna_state_t *state, uint8_t *data, uint8_t length,
                        uint8_t source, uint8_t pool);

#ifdef __cplusplus
}
#endif

#endif /* FORTUNA_H */
/** @} */
