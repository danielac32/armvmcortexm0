/* STM32-like linker script for ARMv6-M simulator */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

_estack = ORIGIN(SRAM) + LENGTH(SRAM);
ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table + code in FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
    } > FLASH

    _etext = .;

    /* Initialized data in SRAM (copied from flash by startup) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        _edata = .;
    } > SRAM AT> FLASH

    /* Zero-initialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

    /* Stack at top of SRAM */
    .stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . += 0x2000; /* 8KB stack */
        _estack = .;
    } > SRAM
}
