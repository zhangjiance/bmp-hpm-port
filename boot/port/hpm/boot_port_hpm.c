/*
 * Boot Port Implementation for HPM5301
 *
 * System control functions for RISC-V HPM5301 MCU
 */

#include "port/boot_port.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "board.h"
#include "boot_log.h"

/* Magic values for bootloader entry request stored in RAM */
#define BOOTMAGIC0           0xb007da7a
#define BOOTMAGIC1           0xbaadfeed
#define BOOT_MAGIC_ADDRESS   0xF0400000  /* AHB SRAM start */
/* Software reset flag in PPOR (bit 31) — survives reset, no RAM needed */
#define PPOR_SW_RESET_FLAG   (1UL << 31)

static volatile uint32_t *boot_magic_ptr = (volatile uint32_t *)BOOT_MAGIC_ADDRESS;

void boot_port_jump_to_app(void)
{
    uint32_t app_addr = 0x80010000;

    BOOT_PRINTF("[BOOT] Jumping to application at 0x%08lx\r\n", app_addr);

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    fencei();

    __asm volatile (
        "jr   %0\n"
        :
        : "r" (app_addr)
        :
    );

    while (1);
}

void boot_port_system_reset(void)
{
    BOOT_PRINTF("[BOOT] System reset requested\r\n");
    board_delay_ms(100);

    HPM_PPOR->RESET_ENABLE = 0x80000000UL;
    HPM_PPOR->SOFTWARE_RESET = 0x1000;

    while (1);
}

void boot_port_disable_irq(void)
{
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

void boot_port_enable_irq(void)
{
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

bool boot_port_check_bootloader_request(void)
{
    extern bool boot_port_board_read_bootpin(void);
    if (boot_port_board_read_bootpin()) {
        BOOT_PRINTF("[BOOT] Boot pin active, staying in bootloader\r\n");
        return true;
    }

    /* Check PPOR software reset flag (set by dfu-util -e / platform_request_boot) */
    if (HPM_PPOR->RESET_FLAG & PPOR_SW_RESET_FLAG) {
        BOOT_PRINTF("[BOOT] Software reset detected (dfu-util -e), staying in bootloader\r\n");
        /* Clear the flag for next boot */
        HPM_PPOR->RESET_FLAG = PPOR_SW_RESET_FLAG;
        return true;
    }

    /* Check magic RAM values */
    if (boot_magic_ptr[0] == BOOTMAGIC0 && boot_magic_ptr[1] == BOOTMAGIC1) {
        BOOT_PRINTF("[BOOT] Magic values found, staying in bootloader\r\n");
        boot_port_clear_bootloader_request();
        return true;
    }

    extern bool boot_flash_port_check_app_valid(void);
    if (!boot_flash_port_check_app_valid()) {
        BOOT_PRINTF("[BOOT] No valid application, staying in bootloader\r\n");
        return true;
    }

    return false;
}

void boot_port_clear_bootloader_request(void)
{
    boot_magic_ptr[0] = 0;
    boot_magic_ptr[1] = 0;
}

void boot_port_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}

extern bool boot_upgrade_request_active(void);
extern void boot_upgrade_request_clear(void);
