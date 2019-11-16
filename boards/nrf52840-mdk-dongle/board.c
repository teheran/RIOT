/*
 * Copyright (C) 2019 Dmitry Korotin <dmitry.korotin@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_nrf52840-mdk-dongle
 * @{
 *
 * @file
 * @brief       Board initialization for the nRF52840-mdk-dongle
 *
 * @author      Dmitry Korotin <dmitry.korotin@gmail.com>
 *
 * @}
 */

#include "cpu.h"
#include "board.h"

#include "periph/gpio.h"

void board_init(void)
{
    /* initialize the board's LEDs */
    gpio_init(LED0_PIN, GPIO_OUT);
    gpio_set(LED0_PIN);
    gpio_init(LED1_PIN, GPIO_OUT);
    gpio_set(LED1_PIN);
    gpio_init(LED2_PIN, GPIO_OUT);
    gpio_set(LED2_PIN);

    /* initialize the board's button */
    gpio_init(BTN0_PIN, GPIO_IN_PU);
    gpio_set(BTN0_PIN);

    /* initialize the CPU */
    cpu_init();
}
