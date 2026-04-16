/*
 * Bootloader Main Application
 * 
 * This file contains the bootloader main logic, which is hardware-independent.
 * All platform-specific operations are handled through the port layer.
 */

#include "board.h"
#include "bootuf2.h"
#include "boot_log.h"

/* Port layer interfaces */
#include "port/boot_port.h"
#include "port/boot_port_board.h"

/* CherryUSB */
#ifndef CONFIG_USB_HS
#define HPM_USB_BASE HPM_USB0_BASE
#else
#define HPM_USB_BASE HPM_USB0_BASE  /* HPM5301 only has USB0 */
#endif

int main(void)
{
    /* Initialize board-level hardware */
    boot_port_board_init();
    
    BOOT_PRINTF("\r\n\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("  HPM UF2 Bootloader\r\n");
    BOOT_PRINTF("  Version: 1.0.0\r\n");
    BOOT_PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    BOOT_PRINTF("  Device: HPM5301\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("Board initialization complete\r\n");
    
    bool stay_in_bootloader = boot_port_check_bootloader_request();
    if (!stay_in_bootloader) {
        BOOT_PRINTF("Valid application found, jumping to app\r\n");
        boot_port_board_deinit();
        boot_port_jump_to_app();
    }

    BOOT_PRINTF("Staying in bootloader mode\r\n");
    BOOT_PRINTF("Initializing USB MSC+DFU...\r\n");
    
    /* Initialize USB MSC with UF2 support */
    msc_bootuf2_init(0, (uintptr_t)HPM_USB_BASE);
    
    BOOT_PRINTF("USB initialized, waiting for connection...\r\n");
    BOOT_PRINTF("========================================\r\n\r\n");
    
    static bool upgrade_done_triggered = false;
    uint32_t heartbeat_ms = 0;
    
    /* Main bootloader loop */
    while (1)
    {
        /* Check DFU WebUSB reset requests */
        dfu_check_reset();
        
        /* Check if firmware upgrade is complete */
        bool req_active = boot_upgrade_request_active();
        bool write_done = bootuf2_is_write_done();
        
        if (!upgrade_done_triggered && req_active && write_done) {
            upgrade_done_triggered = true;
            boot_upgrade_request_clear();
            
            BOOT_PRINTF("\r\n========================================\r\n");
            BOOT_PRINTF("  Firmware Upgrade Complete!\r\n");
            BOOT_PRINTF("  Jumping to application...\r\n");
            BOOT_PRINTF("========================================\r\n\r\n");
            board_delay_ms(100);
            boot_port_board_deinit();
            boot_port_jump_to_app();
            /* Never returns */
        }

        heartbeat_ms++;
        if (heartbeat_ms >= 1000U) {
            heartbeat_ms = 0;
            BOOT_PRINTF("[BOOT] heartbeat: waiting for DFU/MSC\r\n");
        }
        
        board_delay_ms(1);
    }
}
