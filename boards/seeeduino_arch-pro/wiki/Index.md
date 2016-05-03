# Seeeduino Arch Pro

## Overview
The Seeeduino Arch Pro is an 32-bit ARM Cortex M3 microprocessor development board, featuring an Arduino-compatible shield interface and a RJ-45 network interface.

## Hardware
![Seeeduino Arch Pro Development Kit](seeduino_arch-pro.jpg)

### MCU
| MCU             | LPC1768                                  |
|-----------------|------------------------------------------|
| Family          | ARM Cortex-M3                            |
| Vendor          | NXP                                      |
| RAM             | 64KB                                     |
| Flash           | 512KB                                    |
| EEPROM          | no                                       |
| Frequency       | up to 100 MHz                            |
| FPU             | no                                       |
| DMA             | 8 channels                               |
| Timers          | 4 x 32-bit                               |
| ADCs            | 12-bit ADC                               |
| UARTs           | 3x USART                                 |
| SPIs            | 2x USART                                 |
| I2Cs            | 2x                                       |
| Vcc             | 2.4V - 3.6V                              |
| Datasheet       | [Datasheet](http://www.nxp.com/documents/data_sheet/LPC1768_66_65_64.pdf)            |
| Manual          | [Manual](http://www.nxp.com/documents/user_manual/UM10360.pdf)                       |
| Board Manual    | [Board Manual](http://www.seeedstudio.com/wiki/Arch_Pro)                             |
| Board Schematic | [Board Schematic](http://www.seeedstudio.com/wiki/File:Arch_Pro_V1.0_Schematic.pdf)  |

### Pinout

### User interface
| Peripheral | Mapped to | Hardware | Pin   | Comments   |
|------------|-----------|----------|-------|------------|
| LED        | LED0      | LED1     | P1_18 | Green LED  |
|            | LED1      | LED2     | P1_20 | Red LED    |
|            | LED2      | LED3     | P1_21 | Blue LED   |
|            | LED3      | LED4     | P1_23 | Yellow LED |

## Flashing the device
To flash, the [OpenOCD Software](http://openocd.org/) is required.

Flashing is supported by RIOT-OS by the command below:

```
make flash
```

To run the GDB debugger, use the command:

```
make debug
```

Or, to connect with your own debugger:

```
make debug-server
```

## Supported Toolchains
For using the Seeeduino Arch Pro development kit we strongly recommend the usage of the [GNU Tools for ARM Embedded Processors](https://launchpad.net/gcc-arm-embedded) toolchain.

## License information
* Image: taken from Seeeduino website.
