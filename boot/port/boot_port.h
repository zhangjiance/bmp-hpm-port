/*
 * Boot Port Interface
 * 
 * Hardware abstraction layer for bootloader core operations.
 * All platform-specific system control code should be isolated behind these interfaces.
 */

#ifndef BOOT_PORT_H
#define BOOT_PORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// System Control Interface
// ============================================================================

/**
 * @brief Jump from bootloader to application
 * 
 * This function performs the necessary steps to transfer control from
 * bootloader to the application:
 * - Disable interrupts
 * - Reset peripherals
 * - Reconfigure vector table
 * - Set stack pointer
 * - Jump to application entry point
 * 
 * @note This function does NOT return
 */
void boot_port_jump_to_app(void) __attribute__((noreturn));

/**
 * @brief Perform system reset
 * 
 * @note This function does NOT return
 */
void boot_port_system_reset(void) __attribute__((noreturn));

/**
 * @brief Disable all interrupts
 */
void boot_port_disable_irq(void);

/**
 * @brief Enable all interrupts
 */
void boot_port_enable_irq(void);

// ============================================================================
// Bootloader Entry Control Interface
// ============================================================================

/**
 * @brief Check if bootloader entry is requested
 * @return true if bootloader should stay active, false if should jump to app
 * 
 * This function checks multiple conditions:
 * - Hardware boot pin state (e.g., BOOT button)
 * - RAM magic value for software-triggered entry
 * - Application validity
 */
bool boot_port_check_bootloader_request(void);

/**
 * @brief Clear bootloader entry request (e.g., RAM magic)
 */
void boot_port_clear_bootloader_request(void);

// ============================================================================
// Timing Interface
// ============================================================================

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 */
void boot_port_delay_ms(uint32_t ms);

// ============================================================================
// Upgrade Request Interface
// ============================================================================

/**
 * @brief Check if firmware upgrade request is active
 * @return true if upgrade is in progress
 */
bool boot_upgrade_request_active(void);

/**
 * @brief Clear firmware upgrade request flag
 */
void boot_upgrade_request_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_PORT_H */
