/**
 * @brief Fortuna PRNG.
 *
 * The MIT License is applied to this software, see LICENSE.txt
 */

#include "fortuna.h"

static inline void fortuna_increment_counter(fortuna_state_t *state)
{
    state->gen.counter.split.l++;

    if (state->gen.counter.split.l == 0) {
        state->gen.counter.split.h++;
    }
}

/*
 * Corresponds to section 9.4.2.
 */
static void fortuna_reseed(fortuna_state_t *state, uint8_t *seed, size_t length)
{
    sha256_context_t ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, state->gen.key, 32);
    sha256_update(&ctx, seed, length);
    sha256_final(state->gen.key, &ctx);

    fortuna_increment_counter(state);
}

/*
 * Corresponds to section 9.4.3.
 */
static int fortuna_generate_blocks(fortuna_state_t *state, uint8_t *out, size_t blocks)
{
    cipher_context_t cipher;

    /* check if generator has been seeded */
    if (state->gen.counter.split.l == 0 && state->gen.counter.split.h == 0) {
        return -1;
    }

    aes_init(&cipher, state->gen.key, 16);

    for (size_t i = 0; i < blocks; i++) {
        aes_encrypt(&cipher, state->gen.counter.bytes, out + i * 16);
        fortuna_increment_counter(state);
    }

    return 0;
}

/*
 * Corresponds to section 9.4.4.
 */
static int fortuna_generate_data(fortuna_state_t *state, uint8_t *out, size_t bytes)
{
    uint8_t buf[16];

    /* don't touch state when not generating data */
    if (!bytes) {
        return 0;
    }

    /* maximum number of bytes per read is 1MB */
    if (bytes > (1 << 20)) {
        return -1;
    }

    /* generate blocks per 16 bytes */
    size_t blocks = bytes / 16;

    if (fortuna_generate_blocks(state, out, blocks)) {
        return -2;
    }

    /* generate one block for remaining bytes */
    size_t remaining = bytes % 16;

    if (remaining) {
        if (fortuna_generate_blocks(state, buf, 1)) {
            return -2;
        }

        memcpy(out + (blocks * 16), buf, remaining);
    }

    /* switch to a new key to avoid later compromises of this output */
    fortuna_generate_blocks(state, state->gen.key, 2);

    return 0;
}

/*
 * Corresponds to section 9.5.4.
 */
int fortuna_init(fortuna_state_t *state)
{
    memset(state, 0, sizeof(fortuna_state_t));

    for (size_t i = 0; i < FORTUNA_POOLS; i++) {
        sha256_init(&state->pools[i].ctx);
    }

    return 0;
}

/*
 * Corresponds to section 9.5.5.
 */
int fortuna_read(fortuna_state_t *state, uint8_t *out, size_t bytes)
{
    uint8_t buf[FORTUNA_POOLS * 32];

#if FORTUNA_RESEED_TIMEOUT > 0
    uint64_t diff = xtimer_now64() - state->last_reseed;

    if (state->pools[0].len >= 64 && diff > FORTUNA_RESEED_TIMEOUT) {
        state->last_reseed = xtimer_now64();
#else
    if (state->pools[0].len >= 64) {
#endif
        state->reseeds++;
        size_t len = 0;

        for (size_t i = 0; i < FORTUNA_POOLS; i++) {
            if (state->reseeds | (1 << i)) {
                sha256_final(&buf[len], &state->pools[i].ctx);
                sha256_init(&state->pools[i].ctx);
                state->pools[i].len = 0;
                len += 32;
            }
        }

        fortuna_reseed(state, buf, len);
    }

    /* cannot read from unseeded generator */
    if (state->reseeds == 0) {
        return -1;
    }

    return fortuna_generate_data(state, out, bytes);
}

/*
 * Corresponds to section 9.5.6.
 */
int fortuna_add_entropy(fortuna_state_t *state, uint8_t *data, uint8_t length,
                        uint8_t source, uint8_t pool)
{
    if (length < 1 || length > 32) {
        return -1;
    }

    if (pool >= FORTUNA_POOLS) {
        return -2;
    }

    uint8_t header[2];
    header[0] = source;
    header[1] = length;
    sha256_update(&state->pools[pool].ctx, header, 2);
    sha256_update(&state->pools[pool].ctx, (uint8_t *) data, length);
    state->pools[pool].len += length;

    return 0;
}
