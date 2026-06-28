/*
 * Minimal DFU-only USB descriptors for HPM5301 DFU Bootloader.
 * Based on BMProven pattern from ethercat_canfd.
 * USB 2.0 Full-Speed only.
 */
#include "usbd_core.h"
#include "usbd_dfu.h"
#include "usb_dfu.h"
#include "boot_log.h"
#include "board.h"
#include "usb_config.h"
#include "port/boot_flash_port.h"
#include <string.h>

#define DFU_DESC_TOTAL_LEN (9 + 9)
#define USB_CONFIG_SIZE    (9 + DFU_DESC_TOTAL_LEN)
#define DFU_PROGRESS_LINE  64

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, 0xC0, USBD_MAX_POWER),
    /* DFU Interface 0 */
    0x09, 0x04, 0x00, 0x00, 0x00,
    0xFE, 0x01, 0x02,    /* App-Specific, DFU, DFU mode */
    0x04,                /* iInterface = 4 -> DfuSe memory layout string */
    /* DFU Functional Descriptor */
    0x09, 0x21,          /* bLength, bDescriptorType */
    0x0B,                /* bmAttributes: CanDnload | CanUpload | WillDetach */
    0xFF, 0x00,          /* wDetachTimeout = 255ms */
    0x00, 0x10,          /* wTransferSize = 4096 */
    0x1A, 0x01,          /* bcdDFU = 1.1a */
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },
    "HPMicro",
    "HPM DFU BootLoader",
    "2026062800",
    /* DfuSe memory layout: 240 sectors x 4KB, readable+programmable */
    "@Internal Flash  /0x80010000/240*004Kg",
};

static const uint8_t *device_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const char *string_descriptor_cb(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

/* Device Qualifier (required for USB 2.0) */
static const uint8_t device_quality_descriptor[] = {
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    0x40,
    0x01,
    0x00,
};

static const uint8_t *device_quality_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

/* BOS descriptor with USB 2.0 Extension */
static const uint8_t bos_descriptor_data[] = {
    /* BOS Header */
    0x05,
    USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE,
    0x0C, 0x00,
    0x01,
    /* USB 2.0 Extension Capability */
    0x07,
    0x10,
    0x02,
    0x02, 0x00, 0x00, 0x00,
};

static const struct usb_bos_descriptor bos_descriptor = {
    .string = bos_descriptor_data,
    .string_len = sizeof(bos_descriptor_data),
};

const struct usb_descriptor dfu_descriptor = {
    .device_descriptor_callback          = device_descriptor_cb,
    .config_descriptor_callback          = config_descriptor_cb,
    .device_quality_descriptor_callback  = device_quality_descriptor_cb,
    .other_speed_descriptor_callback     = config_descriptor_cb,
    .string_descriptor_callback          = string_descriptor_cb,
    .msosv2_descriptor = NULL,
    .bos_descriptor    = &bos_descriptor,
};

/* ---------- DFU state tracking ---------- */

static bool upgrade_request_active;
static volatile bool dfu_reset_pending;
static bool dfu_first_write;
static uint32_t dfu_download_address;
static uint32_t dfu_rx_blocks;
static uint32_t dfu_rx_bytes;
static uint32_t dfu_rx_checksum;
static uint32_t dfu_max_write_addr;

bool boot_upgrade_request_active(void)  { return upgrade_request_active; }
void boot_upgrade_request_clear(void)   { upgrade_request_active = false; }

/* ---------- DFU callbacks ---------- */

void usbd_dfu_begin_load(void)
{
    upgrade_request_active = true;
    dfu_download_address = boot_flash_port_get_app_start();
    dfu_rx_blocks = 0;
    dfu_rx_bytes = 0;
    dfu_rx_checksum = 0;
    dfu_max_write_addr = dfu_download_address;
    dfu_first_write = true;
    BOOT_PRINTF("[DFU] begin 0x%08lx\r\n", (unsigned long)dfu_download_address);
}

void usbd_dfu_end_load(void)
{
    BOOT_PRINTF("[DFU] end blk=%lu bytes=%lu checksum=0x%08lx\r\n",
                (unsigned long)dfu_rx_blocks,
                (unsigned long)dfu_rx_bytes,
                (unsigned long)dfu_rx_checksum);
}

void usbd_dfu_reset(void)
{
    BOOT_PRINTF("[DFU] USB reset, total: %lu blocks %lu bytes\r\n",
                (unsigned long)dfu_rx_blocks, (unsigned long)dfu_rx_bytes);
    dfu_reset_pending = true;
}

int usbd_dfu_write(uint16_t block_num, const uint8_t *data, uint16_t length)
{
    if (length == 0) { usbd_dfu_end_load(); return 0; }

    uint32_t addr = (uint32_t)block_num * USBD_DFU_XFER_SIZE + dfu_download_address;
    for (uint32_t i = 0; i < length; i++) { dfu_rx_checksum += data[i]; }
    dfu_rx_blocks++;
    dfu_rx_bytes += length;

    return boot_flash_port_write(addr, data, length);
}

int usbd_dfu_read(uint16_t block_num, const uint8_t *data, uint16_t length, uint16_t *actual_length)
{
    *actual_length = 0;
    return 0;
}

uint8_t *dfu_read_flash(uint8_t *src, uint8_t *dest, uint32_t len)
{
    (void)src;
    memset(dest, 0xFF, len);
    return dest;
}

/* Bootloader region — must never be erased */
#define BOOT_REGION_START  0x80000000
#define BOOT_REGION_END    0x80010000

/* DfuSe ERASE command — erase one 4KB sector, with bootloader protection */
uint16_t dfu_erase_flash(uint32_t addr)
{
    static bool first_erase = true;
    static uint32_t erase_cnt;
    if (first_erase) {
        dfu_first_write = true;
        first_erase = false;
        erase_cnt = 0;
    }

    /* Protect bootloader region from being erased */
    if (addr >= BOOT_REGION_START && addr < BOOT_REGION_END) {
        BOOT_PRINTF("\n[DFU] DENY erase in boot region 0x%08lx\n", (unsigned long)addr);
        return 0;
    }

    int ret = boot_flash_port_erase_sector(addr);
    if (ret != 0) {
        BOOT_PRINTF("\n[DFU] ERASE failed addr=0x%08lx: %d\n", (unsigned long)addr, ret);
        return 0;
    }
    BOOT_PRINTF("e");
    if ((++erase_cnt % DFU_PROGRESS_LINE) == 0U) {
        BOOT_PRINTF("\n");
    }
    return 1;
}

/* ---------- USB init & reset check ---------- */

static struct usbd_interface intf_dfu;

void dfu_boot_init(uint8_t busid, uintptr_t reg_base)
{
    upgrade_request_active = false;
    dfu_reset_pending = false;

    usbd_desc_register(busid, &dfu_descriptor);
    usbd_add_interface(busid, usbd_dfu_init_intf(&intf_dfu));
    usbd_initialize(busid, reg_base, NULL);
}

void dfu_check_reset(void)
{
    if (dfu_reset_pending) {
        dfu_reset_pending = false;
        BOOT_PRINTF("[DFU] USB reset detected, flushing...\r\n");
        fencei();
        board_delay_ms(100);
        extern void boot_port_system_reset(void);
        boot_port_system_reset();
    }
}
