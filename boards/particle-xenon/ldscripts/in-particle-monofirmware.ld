MEMORY
{
    rom (rx)    : ORIGIN =    0x30000, LENGTH = 0xc4000
    ram (w!rx)  : ORIGIN = 0x20000000, LENGTH = 0x40000
}

INCLUDE cortexm_base.ld
