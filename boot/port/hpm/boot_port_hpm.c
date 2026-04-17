/*
 * Boot Port Implementation for HPM Platform
 * 
 * System control functions for RISC-V HPM MCUs
 */

#include "port/boot_port.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "board.h"
#include "boot_log.h"

/* Magic values for bootloader entry request stored in RAM */
#define BOOTMAGIC0           0xb007da7a  /* BlackMagic compatible magic 0 */
#define BOOTMAGIC1           0xbaadfeed  /* BlackMagic compatible magic 1 */
#define BOOT_MAGIC_ADDRESS   0xF0400000  /* AHB SRAM start */

/* Bootloader entry request flag in RAM (preserved across soft reset) */
static volatile uint32_t *boot_magic_ptr = (volatile uint32_t *)BOOT_MAGIC_ADDRESS;

/**
 * @brief Jump to application
 * @note This function does NOT return
 */
void boot_port_jump_to_app(void)
{
    uint32_t app_addr = 0x80020000;  /* Application start address */
    uint32_t app_entry = app_addr + 4U;
    
    BOOT_PRINTF("[BOOT] Jumping to application at 0x%08lx (entry: 0x%08lx)\r\n", 
                app_addr, app_entry);
    
    /* Disable interrupts */
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    
    /* Invalidate instruction cache */
    fencei();
    
    /* Jump to application entry (APP start + 4, first word is UF2 signature). */
    __asm volatile (
        "jr   %0\n"
        :
        : "r" (app_entry)
        :
    );
    
    /* Should never reach here */
    while (1);
}

/**
 * @brief Perform system reset
 * @note This function does NOT return
 */
void boot_port_system_reset(void)
{
    BOOT_PRINTF("[BOOT] System reset requested\r\n");
    board_delay_ms(100);
    
    /* HPM system reset */
    HPM_PPOR->RESET_ENABLE = 0x80000000UL;
    HPM_PPOR->SOFTWARE_RESET = 0x1000;
    
    /* Should never reach here */
    while (1);
}

/**
 * @brief Disable all interrupts
 */
void boot_port_disable_irq(void)
{
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

/**
 * @brief Enable all interrupts
 */
void boot_port_enable_irq(void)
{
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

/**
 * @brief Check if bootloader entry is requested
 * @return true if should stay in bootloader, false otherwise
 */
bool boot_port_check_bootloader_request(void)
{
    /* Check hardware boot pin */
    extern bool boot_port_board_read_bootpin(void);
    if (boot_port_board_read_bootpin()) {
        BOOT_PRINTF("[BOOT] Boot pin active, staying in bootloader\r\n");
        return true;
    }
    
    /* Check RAM magic value for software-triggered entry */
    if (boot_magic_ptr[0] == BOOTMAGIC0 && boot_magic_ptr[1] == BOOTMAGIC1) {
        BOOT_PRINTF("[BOOT] Magic values found (0x%08lx, 0x%08lx), staying in bootloader\r\n",
                    (unsigned long)boot_magic_ptr[0], (unsigned long)boot_magic_ptr[1]);
        boot_port_clear_bootloader_request();
        return true;
    }
    
    /* Check if application is valid */
    extern bool boot_flash_port_check_app_valid(void);
    if (!boot_flash_port_check_app_valid()) {
        BOOT_PRINTF("[BOOT] No valid application, staying in bootloader\r\n");
        return true;
    }
    
    return false;
}

/**
 * @brief Clear bootloader entry request
 */
void boot_port_clear_bootloader_request(void)
{
    boot_magic_ptr[0] = 0;
    boot_magic_ptr[1] = 0;
}

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 */
void boot_port_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}

/**
 * @brief Check if firmware upgrade request is active
 * @return true if upgrade is in progress
 * @note Implemented in msc_bootuf2_desc.c
 */
extern bool boot_upgrade_request_active(void);

/**
 * @brief Clear firmware upgrade request flag
 * @note Implemented in msc_bootuf2_desc.c
 */
extern void boot_upgrade_request_clear(void);

/**
 * @brief Set firmware upgrade request flag
 * @note Implemented in msc_bootuf2_desc.c
 */
extern void boot_upgrade_request_set(void);
