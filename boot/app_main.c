/*
 * Bootloader Main Application
 *
 * This file contains the bootloader main logic, which is hardware-independent.
 * All platform-specific operations are handled through the port layer.
 *
 * DFU-only bootloader for HPM5301.
 */

#include "board.h"
#include "boot_log.h"
#include "port/boot_port.h"
#include "port/boot_port_board.h"

#ifndef CONFIG_USB_HS
#define HPM_USB_BASE HPM_USB0_BASE
#else
#define HPM_USB_BASE HPM_USB0_BASE
#endif

int main(void)
{
    boot_port_board_init();

    BOOT_PRINTF("\r\n\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("  HPM DFU Bootloader\r\n");
    BOOT_PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("Board initialization complete\r\n");

    bool stay_in_bootloader = boot_port_check_bootloader_request();
    if (!stay_in_bootloader) {
        BOOT_PRINTF("Valid app, jumping...\r\n");
        boot_port_board_deinit();
        boot_port_jump_to_app();
    }

    BOOT_PRINTF("Staying in bootloader mode\r\n");
    BOOT_PRINTF("Initializing USB DFU...\r\n");

    extern void dfu_boot_init(uint8_t busid, uintptr_t reg_base);
    extern void dfu_check_reset(void);
    dfu_boot_init(0, (uintptr_t)HPM_USB_BASE);

    BOOT_PRINTF("USB initialized, waiting for connection...\r\n");
    BOOT_PRINTF("========================================\r\n\r\n");

    while (1)
    {
        dfu_check_reset();

        board_delay_ms(1);
    }
}
