/*
 * Boot Port Board Implementation for HPM Platform
 * 
 * Board-level initialization for bootloader
 */

#include "port/boot_port_board.h"
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "boot_log.h"

/**
 * @brief Initialize board-level hardware
 */
void boot_port_board_init(void)
{
    /* Initialize USB DP/DM pins first (must be before clock init) */
    board_init_usb_dp_dm_pins();
    
    /* Initialize system clock */
    board_init_clock();
    
    /* Initialize console for debug output */
    board_init_console();
    
    /* Initialize boot pin GPIO */
    boot_port_board_init_bootpin();
    
    /* Initialize LED for visual feedback */
    board_init_led_pins();
    board_led_write(1);  /* Turn on LED to indicate bootloader mode */
    
    /* Initialize USB controller */
    board_init_usb(HPM_USB0);
}

/**
 * @brief Deinitialize peripherals before jumping to application
 */
void boot_port_board_deinit(void)
{
    /* Turn off LED */
    board_led_write(0);
    
    /* Deinitialize USB (if needed) */
    /* Note: USB controller will be re-initialized by application */
}

/**
 * @brief Initialize boot entry control GPIO
 */
void boot_port_board_init_bootpin(void)
{
    /* Boot button GPIO initialization */
#ifdef BOARD_APP_BOOT_BTN_GPIO_CTRL
    /* If boot button is defined, configure it as input */
    board_init_gpio_pins();
#else
    /* No boot button defined, skip initialization */
#endif
}

/**
 * @brief Read boot entry control pin state
 * @return true if boot pin is active (pressed), false otherwise
 */
bool boot_port_board_read_bootpin(void)
{
    /* Read boot button state
     * Boot button is active low on most HPM boards */
#ifdef BOARD_APP_BOOT_BTN_GPIO_CTRL
    uint8_t pin_state = gpio_read_pin(BOARD_APP_BOOT_BTN_GPIO_CTRL, 
                                      BOARD_APP_BOOT_BTN_GPIO_INDEX,
                                      BOARD_APP_BOOT_BTN_GPIO_PIN);
    
    /* Active low - return true if pressed (pin is low) */
    return (pin_state == 0);
#else
    /* No boot button defined - always return false */
    return false;
#endif
}
