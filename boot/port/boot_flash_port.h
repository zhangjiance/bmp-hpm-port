/*
 * Boot Flash Port Interface
 * 
 * Hardware abstraction layer for flash operations during bootloader operation.
 * All platform-specific flash code should be isolated behind these interfaces.
 */

#ifndef BOOT_FLASH_PORT_H
#define BOOT_FLASH_PORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Flash Configuration (provided by platform)
// ============================================================================

/**
 * @brief Get application start address in flash
 * @return Application start address
 */
uint32_t boot_flash_port_get_app_start(void);

/**
 * @brief Get application end address in flash
 * @return Application end address
 */
uint32_t boot_flash_port_get_app_end(void);

/**
 * @brief Get application size in bytes
 * @return Application size
 */
uint32_t boot_flash_port_get_app_size(void);

/**
 * @brief Get flash page size in bytes
 * @return Flash page size
 */
uint32_t boot_flash_port_get_page_size(void);

// ============================================================================
// Flash Operation Interface
// ============================================================================

/**
 * @brief Write data to flash (program only, caller handles erase)
 * @param address Target flash address
 * @param data Source data buffer
 * @param size Number of bytes to write
 * @return 0 on success, negative error code on failure
 *         -1: address out of range
 *         -2: alignment error
 *         -3: program failure
 *         -4: verify failure
 *         -5: internal flash driver error
 */
int boot_flash_port_write(uint32_t address, const uint8_t *data, size_t size);

/**
 * @brief Erase one flash sector at the specified address
 * @param address Any address inside target sector
 * @return 0 on success, negative error code on failure
 */
int boot_flash_port_erase_sector(uint32_t address);

/**
 * @brief Erase entire application region
 * @return 0 on success, negative error code on failure
 *         -1: erase operation failed
 *         -2: erase verification failed
 */
int boot_flash_port_erase_app(void);

/**
 * @brief Check if application in flash is valid
 * @return true if valid application exists, false otherwise
 */
bool boot_flash_port_check_app_valid(void);

/**
 * @brief Accumulate checksum of flash data (for integrity checking)
 * @param addr Flash address
 * @param len Length of data
 */
void boot_flash_port_accumulate_checksum(uint32_t addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_FLASH_PORT_H */
