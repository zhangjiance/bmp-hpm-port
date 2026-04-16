/*
 * Boot Port Board Interface
 * 
 * Hardware abstraction layer for board-level initialization during bootloader.
 * All platform-specific board initialization code should be isolated behind these interfaces.
 */

#ifndef BOOT_PORT_BOARD_H
#define BOOT_PORT_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Board Initialization Interface
// ============================================================================

/**
 * @brief Initialize board-level hardware
 * 
 * This function performs all necessary board-level initialization:
 * - Board initialization
 * - System clock configuration
 * - GPIO initialization
 * - UART initialization (for logging)
 * - Other peripheral initialization as needed
 */
void boot_port_board_init(void);

/**
 * @brief Deinitialize peripherals before jumping to application
 * 
 * This function cleanly shuts down peripherals to ensure clean
 * application startup.
 */
void boot_port_board_deinit(void);

/**
 * @brief Initialize boot entry control GPIO (e.g., BOOT button)
 */
void boot_port_board_init_bootpin(void);

/**
 * @brief Read boot entry control pin state
 * @return true if boot pin is active (stay in bootloader), false otherwise
 */
bool boot_port_board_read_bootpin(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_PORT_BOARD_H */
