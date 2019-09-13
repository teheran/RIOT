/*
 * Copyright (C) 2019 Christian Amsüss <chrysn@fsfe.org>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @addtogroup      boards_nrf52840
 * @{
 *
 * @file
 * @brief           Memory definitions for the nRF52840xe10000-dongle via bootloader
 *
 * This was derived from combinding the nRF52840 memory map data with the
 * observation that the bootloader preparation step (`nrfutil pkg generate`)
 * strips off the first 0x1000 byte part of the hex file, and that working
 * examples start there (the address of the first used part in the hex file,
 * which looks like a vector table, contains addresses that point to right
 * after that vector table).
 *
 * This should be re-done later when the bootloader is better understood.
 *
 * @author          Christian Amsüss <chrysn@fsfe.org>
 *
 * @}
 */

MEMORY
{
    rom (rx)    : ORIGIN =     0x1000, LENGTH = 0xff000
    ram (w!rx)  : ORIGIN = 0x20000000, LENGTH = 0x40000
}

INCLUDE cortexm_base.ld
