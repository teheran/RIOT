/*
 * Copyright (C) 2019 Christian Amsüss <chrysn@fsfe.org>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_nrf52840-dongle
 * @{
 *
 * @file
 * @brief       Board specific configuration for the nRF52840-Dongle
 *
 * @author      Christian Amsüss <chrysn@fsfe.org>
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
 /** @brief The LED labelled LD1 */
#define LED0_PIN            GPIO_PIN(0, 6)

 /** @brief The red channel of the LED labelled LD2 */
#define LED1_PIN            GPIO_PIN(0, 8)
 /** @brief The green channel of the LED labelled LD2 */
#define LED2_PIN            GPIO_PIN(1, 9)
 /** @brief The blue channel of the LED labelled LD2 */
#define LED3_PIN            GPIO_PIN(0, 12)

/** @} */

/**
 * @name    Button pin configuration
 * @{
 */
/** @brief The button labelled SW1 */
#define BTN0_PIN            GPIO_PIN(1, 6)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
