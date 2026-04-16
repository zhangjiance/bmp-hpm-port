/*
 * Boot Flash Port IMPLEMENTATION for HPM Platform
 * 
 * Flash operations using HPM SDK XPI ROM API
 */

#include "port/boot_flash_port.h"
#include "hpm_romapi.h"
#include "hpm_common.h"
#include "board.h"
#include "boot_log.h"

/* Flash configuration from board */
#ifndef BOARD_APP_XPI_NOR_XPI_BASE
#define BOARD_APP_XPI_NOR_XPI_BASE HPM_XPI0_BASE
#endif

/* Application region configuration */
#define BOOT_FLASH_APP_START    0x80020000  /* 128KB bootloader */
#define BOOT_FLASH_APP_SIZE     0xE0000     /* 896KB application */
#define BOOT_FLASH_APP_END      (BOOT_FLASH_APP_START + BOOT_FLASH_APP_SIZE)
#define BOOT_FLASH_PAGE_SIZE    4096        /* 4KB sector size */

/* Global XPI NOR config */
static xpi_nor_config_t s_xpi_nor_config;
static bool s_xpi_initialized = false;

static void boot_flash_init_xpi(void)
{
    if (s_xpi_initialized) {
        return;
    }

    /* Initialize XPI NOR configuration option */
    xpi_nor_config_option_t cfg_opt;
    cfg_opt.header.U = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    cfg_opt.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    cfg_opt.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;

    /* Initialize XPI NOR */
    hpm_stat_t status = rom_xpi_nor_auto_config(BOARD_APP_XPI_NOR_XPI_BASE, &s_xpi_nor_config, &cfg_opt);
    if (status != status_success) {
        BOOT_PRINTF("[FLASH] XPI auto config failed: %d\r\n", status);
        return;
    }

    s_xpi_initialized = true;
    BOOT_PRINTF("[FLASH] XPI initialized\r\n");
}

uint32_t boot_flash_port_get_app_start(void)
{
    return BOOT_FLASH_APP_START;
}

uint32_t boot_flash_port_get_app_end(void)
{
    return BOOT_FLASH_APP_END;
}

uint32_t boot_flash_port_get_app_size(void)
{
    return BOOT_FLASH_APP_SIZE;
}

uint32_t boot_flash_port_get_page_size(void)
{
    return BOOT_FLASH_PAGE_SIZE;
}

int boot_flash_port_write(uint32_t address, const uint8_t *data, size_t size)
{
    boot_flash_init_xpi();

    /* HPM ROM API requires 4-byte aligned flash address */
    if ((address % 4) != 0) {
        return -2;
    }

    if (size == 0) {
        return 0;
    }

    /* Check address range */
    if (address < BOOT_FLASH_APP_START || (address + size) > BOOT_FLASH_APP_END) {
        return -1;
    }

    /*
     * Some DFU paths can pass buffers/sizes that are not 4-byte aligned.
     * Stage data into a local aligned buffer and pad tail with 0xFF.
     */
    uint32_t staging[64]; /* 256-byte chunk, 4-byte aligned */
    uint32_t curr_addr = address;
    size_t remaining = size;
    const uint8_t *src = data;

    while (remaining > 0) {
        size_t chunk = remaining;
        if (chunk > sizeof(staging)) {
            chunk = sizeof(staging);
        }

        size_t aligned_chunk = (chunk + 3U) & ~((size_t)3U);

        memset(staging, 0xFF, aligned_chunk);
        memcpy(staging, src, chunk);

        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        hpm_stat_t status = rom_xpi_nor_program(BOARD_APP_XPI_NOR_XPI_BASE,
                                                xpi_xfer_channel_auto,
                                                &s_xpi_nor_config,
                                                staging,
                            curr_addr - BOARD_FLASH_BASE_ADDRESS,
                                                aligned_chunk);
        enable_global_irq(CSR_MSTATUS_MIE_MASK);

        if (status != status_success) {
            BOOT_PRINTF("[FLASH] Program 0x%08lx len %lu(aligned %lu) failed: %d\r\n",
                        curr_addr,
                        (unsigned long)chunk,
                        (unsigned long)aligned_chunk,
                        status);
            return -3;
        }

        curr_addr += chunk;
        src += chunk;
        remaining -= chunk;
    }

    return 0;
}

int boot_flash_port_erase_sector(uint32_t address)
{
    boot_flash_init_xpi();

    if (address < BOOT_FLASH_APP_START || address >= BOOT_FLASH_APP_END) {
        return -1;
    }

    uint32_t sector_addr = address & ~(BOOT_FLASH_PAGE_SIZE - 1);

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    hpm_stat_t status = rom_xpi_nor_erase_sector(BOARD_APP_XPI_NOR_XPI_BASE,
                                                 xpi_xfer_channel_auto,
                                                 &s_xpi_nor_config,
                                                 sector_addr - BOARD_FLASH_BASE_ADDRESS);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);

    if (status != status_success) {
        BOOT_PRINTF("[FLASH] Erase sector 0x%08lx failed: %d\r\n", sector_addr, status);
        return -2;
    }

    return 0;
}

int boot_flash_port_erase_app(void)
{
    boot_flash_init_xpi();

    BOOT_PRINTF("[FLASH] Erasing application region 0x%08lx - 0x%08lx...\r\n",
                BOOT_FLASH_APP_START, BOOT_FLASH_APP_END);

    /* Erase application region sector by sector */
    for (uint32_t addr = BOOT_FLASH_APP_START; addr < BOOT_FLASH_APP_END; addr += BOOT_FLASH_PAGE_SIZE) {
        hpm_stat_t status = rom_xpi_nor_erase_sector(BOARD_APP_XPI_NOR_XPI_BASE,
                                                     xpi_xfer_channel_auto,
                                                     &s_xpi_nor_config,
                                                     addr - BOARD_FLASH_BASE_ADDRESS);
        if (status != status_success) {
            BOOT_PRINTF("[FLASH] Erase sector 0x%08lx failed: %d\r\n", addr, status);
            return -1;
        }

        /* Print progress every 64KB */
        if ((addr - BOOT_FLASH_APP_START) % (64 * 1024) == 0) {
            BOOT_PRINTF("[FLASH] Erased %lu KB\r\n", (addr - BOOT_FLASH_APP_START) / 1024);
        }
    }

    /*
     * Verify erase by reading back through ROM API.
     * This avoids stale/aliased data from memory-mapped XIP reads.
     */
    uint32_t verify_buf[64]; /* 256-byte aligned read buffer */
    for (uint32_t verify_addr = BOOT_FLASH_APP_START; verify_addr < BOOT_FLASH_APP_END; verify_addr += sizeof(verify_buf)) {
        uint32_t read_len = (uint32_t)sizeof(verify_buf);
        if ((verify_addr + read_len) > BOOT_FLASH_APP_END) {
            read_len = BOOT_FLASH_APP_END - verify_addr;
        }

        hpm_stat_t read_status = rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE,
                                                  xpi_xfer_channel_auto,
                                                  &s_xpi_nor_config,
                                                  verify_buf,
                                                  verify_addr - BOARD_FLASH_BASE_ADDRESS,
                                                  read_len);
        if (read_status != status_success) {
            BOOT_PRINTF("[FLASH] Erase verify read failed at 0x%08lx: %d\r\n",
                        verify_addr,
                        read_status);
            return -3;
        }

        const uint8_t *verify_bytes = (const uint8_t *)verify_buf;
        for (uint32_t i = 0; i < read_len; i++) {
            if (verify_bytes[i] != 0xFFU) {
                BOOT_PRINTF("[FLASH] Erase verification failed at offset 0x%08lx\r\n",
                            verify_addr + i);
                return -2;
            }
        }
    }

    BOOT_PRINTF("[FLASH] Application region erased successfully\r\n");
    return 0;
}

bool boot_flash_port_check_app_valid(void)
{
    /* Force fence to ensure flash reads are not cached */
    fencei();
    
    const uint32_t signature = *(const uint32_t *)BOOT_FLASH_APP_START;
    const uint32_t expected = BOARD_UF2_SIGNATURE;
    
    BOOT_PRINTF("[BOOT] Checking app signature at 0x%08lx\r\n", BOOT_FLASH_APP_START);
    BOOT_PRINTF("[BOOT]   Read:     0x%08lx\r\n", signature);
    BOOT_PRINTF("[BOOT]   Expected: 0x%08lx\r\n", expected);
    BOOT_PRINTF("[BOOT]   Valid:    %s\r\n", (signature == expected) ? "YES" : "NO");
    
    return signature == BOARD_UF2_SIGNATURE;
}

void boot_flash_port_accumulate_checksum(uint32_t addr, size_t len)
{
    /* Checksum accumulation is handled in bootuf2.c */
    /* This is just a placeholder for port compatibility */
    (void)addr;
    (void)len;
}
