/*
 * Simple DFU-Only Bootloader Main
 * 
 * Minimalist bootloader implementation for USB enumeration testing
 */

#include "board.h"
#include "boot_log.h"
#include "usb_config.h"
#include "hpm_interrupt.h"

/* Simple DFU interface */
extern void simple_dfu_init(uint8_t busid, uintptr_t reg_base);

int main(void)
{
    board_init();
    board_init_led_pins();
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    
    BOOT_PRINTF("\r\n\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("  HPM Simple DFU Bootloader\r\n");
    BOOT_PRINTF("  Version: 1.0.0 (Minimal)\r\n");
    BOOT_PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    BOOT_PRINTF("  Device: HPM5301\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("Board/USB initialization complete\r\n");
    
    BOOT_PRINTF("Bootloader mode active!\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("Initializing USB DFU...\r\n");
    
    /* Initialize USB DFU (simple, DFU-only mode) */
    simple_dfu_init(0, (uintptr_t)CONFIG_HPM_USBD_BASE);
    
    BOOT_PRINTF("USB DFU ready - device should enumerate now\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("\r\n");
    BOOT_PRINTF("Expected USB device:\r\n");
    BOOT_PRINTF("  VID: 0x34B7 (HPMicro)\r\n");
    BOOT_PRINTF("  PID: 0x1002 (DFU Device)\r\n");
    BOOT_PRINTF("  Class: DFU (0xFE)\r\n");
    BOOT_PRINTF("\r\n");
    BOOT_PRINTF("Check with:\r\n");
    BOOT_PRINTF("  Linux:   lsusb | grep 34B7\r\n");
    BOOT_PRINTF("  Windows: Check Device Manager\r\n");
    BOOT_PRINTF("\r\n");
    BOOT_PRINTF("========================================\r\n");
    BOOT_PRINTF("Entering main loop...\r\n\r\n");
    
    uint32_t loop_count = 0;
    
    /* Main bootloader loop */
    while (1)
    {
        /* Print heartbeat every 5 seconds */
        if ((loop_count % 5000) == 0) {
            BOOT_PRINTF("[%lu sec] Bootloader running, waiting for DFU commands...\r\n", 
                        loop_count / 1000);
        }
        
        board_delay_ms(1);
        loop_count++;
    }
}
