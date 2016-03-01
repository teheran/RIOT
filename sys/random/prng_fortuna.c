/**
 * Copyright (C) 2016 Bas Stottelaar <basstottelaar@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup sys_random
 * @{
 * @file
 *
 * @brief Glue-code for Fortuna PRNG.
 *
 * @author Bas Stottelaar <basstottelaar@gmail.com>
 * @}
 */

#include "fortuna/fortuna.h"

/**
 * @brief This holds the PRNG state.
 */
static fortuna_state_t fortuna_state;

void random_init(uint32_t s)
{
    int i;

    /* setup PRNG state */
    fortuna_init(&fortuna_state);

    /* not optimal because a seed of 32 bits is not sufficient to seed all of
       the pools with at least 64 bytes */
    for (i = 0; i < FORTUNA_POOLS; i++) {
        fortuna_add_entropy(&fortuna_state, (uint8_t *) &s, 4, 0, 0);
    }
}

uint32_t random_uint32(void)
{
    uint32_t data;

    fortuna_read(&fortuna_state, (uint8_t *) &data, 4);

    return data;
}
