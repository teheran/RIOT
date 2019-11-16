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
 * @brief       Board specific configuration for the nRF52840-MDK-Dongle
 *
 * @author      Dmitry Korotin <dmitry.korotin@gmail.com>
 */

#ifndef BOARD_H
#define BOARD_H

#include "cpu.h"
#include "board_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    LED pin configuration
 * @{
 */
 /** @brief The red channel of the LED labelled D6 */
#define LED0_PIN            GPIO_PIN(0, 23)
 /** @brief The green channel of the LED labelled D6 */
#define LED1_PIN            GPIO_PIN(0, 22)
 /** @brief The blue channel of the LED labelled D6 */
#define LED2_PIN            GPIO_PIN(0, 24)

/** @} */

/**
 * @name    Button pin configuration
 * @{
 */
/** @brief The button labelled SW1 */
#define BTN0_PIN            GPIO_PIN(0, 18)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
