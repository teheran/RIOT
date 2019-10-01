/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_common_particle-mesh
 * @{
 *
 * @file
 * @brief       Common board initialization for the Particle Mesh
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include "cpu.h"
#include "board.h"

#include "periph/gpio.h"

void board_nrfantenna_select(enum board_nrfantenna_selection choice)
{
    switch (choice) {
        case BOARD_NRFANTENNA_BUILTIN:
            /* Suppress output to the UFL connector */
            gpio_set(VCTL1_PIN);
#ifdef VCTL2_PIN
            /* Enable output to the built-in antenna */
            gpio_clear(VCTL2_PIN);
#endif
            break;
        case BOARD_NRFANTENNA_EXTERNAL:
            gpio_clear(VCTL1_PIN);
#ifdef VCTL2_PIN
            gpio_set(VCTL2_PIN);
#endif
            break;
    }
}

void board_init(void)
{
#ifdef PARTICLE_MONOFIRMWARE
    // Set VTOR address to where our vector table is located (as does micropython)
    // FIXME:
    // * Use the abstractions (probbably this is the SCB->VTOR = _isr_Vectors
    //   already present in cortexm_init.c
    // * Find out whether this is actually required
    // * If so, consider whether there's any benefit to an unpinned vectro
    //   table now that we need to set it ourselves anyway. (Note that Particle
    //   bootloader still appears to look into two magic locations: the
    //   firmware start to see whether it contains something that looks like a
    //   stack start address and a reset vector to jump into, and the
    //   module_info_t at offset 0x200).
    *((volatile uint32_t*)0xe000ed08) = 0x30000;

    // Force keeping this sections -- the __attribute__((used)) in their macro
    // expansions should do that, need to investigate.
    extern uint32_t particle_monofirmware_padding;
    extern uint32_t particle_monofirmware_module_info;
    volatile uint32_t x;
    x = (uint32_t)&particle_monofirmware_padding;
    x = (uint32_t)&particle_monofirmware_module_info;
    (void)x;

    // Clear out POWER_CLOCK and GPIOTE interrupts set by the bootloader. (If
    // something does enable them, it'll do so after the board_init call).
    NVIC_DisableIRQ(0);
    NVIC_DisableIRQ(6);
#endif

    /* initialize the boards LEDs */
    gpio_init(LED0_PIN, GPIO_OUT);
    gpio_set(LED0_PIN);
    gpio_init(LED1_PIN, GPIO_OUT);
    gpio_set(LED1_PIN);
    gpio_init(LED2_PIN, GPIO_OUT);
    gpio_set(LED2_PIN);

    gpio_init(VCTL1_PIN, VCTLn_MODE);
#ifdef VCTL2_PIN
    /* On boards without VCLT2_PIN (Boron), the VCTL2 net is driven by NOT(VCTL1) */
    gpio_init(VCTL2_PIN, VCTLn_MODE);
#endif

    board_nrfantenna_select(BOARD_NRFANTENNA_DEFAULT);

    /* initialize the CPU */
    cpu_init();
}
