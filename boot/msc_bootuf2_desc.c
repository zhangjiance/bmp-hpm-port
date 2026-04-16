/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_msc.h"
#include "usbd_dfu.h"
#include "usb_dfu.h"
#include "bootuf2.h"
#include "boot_log.h"
#include "board.h"
#include <string.h>

/* Port layer interface for flash operations */
#include "port/boot_flash_port.h"

#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

#define USBD_VID           0x34BF  /* HPMicro VID */
#define USBD_PID           0x0003  /* UF2 Bootloader PID */
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define WINUSB_VENDOR_CODE          0x20
#define DFU_INTERFACE_NUMBER        0x01
#define DFU_INTERFACE_DESCRIPTOR_LEN 9
#define DFU_FUNCTIONAL_DESCRIPTOR_LEN 9
#define DFU_DESCRIPTOR_LEN          (DFU_INTERFACE_DESCRIPTOR_LEN + DFU_FUNCTIONAL_DESCRIPTOR_LEN)
#define DFU_MSOSV2_DESCRIPTOR_LEN   (10 + USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_LEN)

#define USB_CONFIG_SIZE (9 + MSC_DESCRIPTOR_LEN + DFU_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define MSC_MAX_MPS 512
#else
#define MSC_MAX_MPS 64
#endif

static volatile bool upgrade_request_active;

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t dfu_winusb_msosv2_desc_set[] = {
    USB_MSOSV2_COMP_ID_SET_HEADER_DESCRIPTOR_INIT(DFU_MSOSV2_DESCRIPTOR_LEN),
    USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_INIT(DFU_INTERFACE_NUMBER),
};

static const struct usb_msosv2_descriptor msosv2_descriptor = {
    .vendor_code = WINUSB_VENDOR_CODE,
    .compat_id = dfu_winusb_msosv2_desc_set,
    .compat_id_len = sizeof(dfu_winusb_msosv2_desc_set),
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x02),
    
    /* DFU Interface Descriptor (Interface 1) */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    0x01,                          /* bInterfaceNumber = 1 (MSC is 0) */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x02,                          /* bInterfaceProtocol (DFU mode) */
    0x04,                          /* iInterface (String Index 4) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x07,                          /* bmAttributes (bitCanDnload | bitWillDetach | bitManifestationTolerant) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x02,                    /* wTransferSize = 512 bytes */
    0x10, 0x01                     /* bcdDFUVersion = 1.1 */
};

static const uint8_t device_quality_descriptor[] = {
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
};

/* USB 2.0 Extension BOS Descriptor */
static const uint8_t bos_descriptor_data[] = {
    /* BOS Header */
    0x05,                          /* bLength */
    USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE, /* bDescriptorType */
    0x28, 0x00,                    /* wTotalLength: 40 bytes */
    0x02,                          /* bNumDeviceCaps: 2 */
    
    /* USB 2.0 Extension Capability */
    0x07,                          /* bLength */
    0x10,                          /* bDescriptorType: DEVICE CAPABILITY */
    0x02,                          /* bDevCapabilityType: USB 2.0 EXTENSION */
    0x02, 0x00, 0x00, 0x00,        /* bmAttributes: LPM supported (bit 1) */

    /* Microsoft OS 2.0 Platform Capability (WinUSB) */
    USB_BOS_CAP_PLATFORM_WINUSB_DESCRIPTOR_INIT(WINUSB_VENDOR_CODE, sizeof(dfu_winusb_msosv2_desc_set))
};

static const struct usb_bos_descriptor bos_descriptor = {
    .string = bos_descriptor_data,
    .string_len = sizeof(bos_descriptor_data)
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },  /* Langid */
    "HPMicro",                      /* Manufacturer */
    "HPM UF2+DFU Bootloader",       /* Product */
    "2024041500",                   /* Serial Number */
    "HPM DFU Device",               /* DFU Interface */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    if (index > 4) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor msc_bootuf2_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .msosv2_descriptor = &msosv2_descriptor,
    .bos_descriptor = &bos_descriptor,
    .string_descriptor_callback = string_descriptor_callback
};

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            BOOT_PRINTF("[USB] EVENT_RESET\r\n");
            break;
        case USBD_EVENT_CONNECTED:
            BOOT_PRINTF("[USB] EVENT_CONNECTED\r\n");
            break;
        case USBD_EVENT_DISCONNECTED:
            BOOT_PRINTF("[USB] EVENT_DISCONNECTED\r\n");
            break;
        case USBD_EVENT_RESUME:
            BOOT_PRINTF("[USB] EVENT_RESUME\r\n");
            break;
        case USBD_EVENT_SUSPEND:
            BOOT_PRINTF("[USB] EVENT_SUSPEND\r\n");
            break;
        case USBD_EVENT_CONFIGURED:
            BOOT_PRINTF("[USB] EVENT_CONFIGURED\r\n");
            bootuf2_init();
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            BOOT_PRINTF("[USB] EVENT_SET_REMOTE_WAKEUP\r\n");
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            BOOT_PRINTF("[USB] EVENT_CLR_REMOTE_WAKEUP\r\n");
            break;

        default:
            BOOT_PRINTF("[USB] EVENT_UNKNOWN: %d\r\n", event);
            break;
    }
}

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    *block_num = bootuf2_get_sector_count();
    *block_size = bootuf2_get_sector_size();

    USB_LOG_INFO("sector count:%d, sector size:%d\r\n", (unsigned int)*block_num, (unsigned int)*block_size);
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    boot2uf2_read_sector(sector, buffer, length / bootuf2_get_sector_size());
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    USB_LOG_DBG("MSC write: sector=%u, length=%u\r\n", (unsigned int)sector, (unsigned int)length);
    bootuf2_write_sector(sector, buffer, length / bootuf2_get_sector_size());
    return 0;
}

static struct usbd_interface intf0;
static struct usbd_interface intf1;

bool boot_upgrade_request_active(void)
{
    return upgrade_request_active;
}

void boot_upgrade_request_clear(void)
{
    upgrade_request_active = false;
}

void boot_upgrade_request_set(void)
{
    upgrade_request_active = true;
}

void msc_bootuf2_init(uint8_t busid, uintptr_t reg_base)
{
    upgrade_request_active = false;
    usbd_desc_register(busid, &msc_bootuf2_descriptor);
    usbd_add_interface(busid, usbd_msc_init_intf(busid, &intf0, MSC_OUT_EP, MSC_IN_EP));
    usbd_add_interface(busid, usbd_dfu_init_intf(&intf1));
    usbd_initialize(busid, reg_base, usbd_event_handler);
}

static uint32_t dfu_download_address;
static volatile bool dfu_erased = false;
static volatile bool dfu_reset_pending = false;
static volatile bool dfu_first_write_logged = false;
static uint32_t dfu_rx_blocks = 0;
static uint32_t dfu_rx_bytes = 0;
static uint32_t dfu_rx_checksum = 0;

static void dfu_path_accumulate(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        dfu_rx_checksum += data[i];
    }
}

void usbd_dfu_begin_load(void)
{
    USB_LOG_INFO("DFU download begin\r\n");
    upgrade_request_active = true;
    dfu_download_address = boot_flash_port_get_app_start();
    dfu_erased = false;
    dfu_first_write_logged = false;
    dfu_rx_blocks = 0;
    dfu_rx_bytes = 0;
    dfu_rx_checksum = 0;
    BOOT_PRINTF("[DFU-PATH] begin, app_start=0x%08lx\r\n", (unsigned long)dfu_download_address);
}

void usbd_dfu_end_load(void)
{
    USB_LOG_INFO("DFU download end\r\n");
    BOOT_PRINTF("[DFU-PATH] end, blocks=%lu bytes=%lu checksum=0x%08lx\r\n",
                (unsigned long)dfu_rx_blocks,
                (unsigned long)dfu_rx_bytes,
                (unsigned long)dfu_rx_checksum);
}

void usbd_dfu_reset(void)
{
    USB_LOG_INFO("[DFU] USB Reset request received\r\n");
    BOOT_PRINTF("[DFU] Total received: %lu blocks, %lu bytes, checksum=0x%08lx\r\n",
                (unsigned long)dfu_rx_blocks,
                (unsigned long)dfu_rx_bytes,
                (unsigned long)dfu_rx_checksum);
    dfu_reset_pending = true;
}

int usbd_dfu_write(uint16_t block_num, const uint8_t *data, uint16_t length)
{
    if (length == 0) {
        usbd_dfu_end_load();
        return 0;
    }

    if (!dfu_erased) {
        int erase_ret = boot_flash_port_erase_app();
        if (erase_ret != 0) {
            USB_LOG_ERR("[DFU] Erase app failed: %d\r\n", erase_ret);
            return erase_ret;
        }
        dfu_erased = true;
    }

    uint32_t addr = (uint32_t)block_num * 512U + dfu_download_address;
    dfu_path_accumulate(data, length);
    dfu_rx_blocks++;
    dfu_rx_bytes += length;
    if (dfu_rx_blocks <= 3U || (dfu_rx_blocks % 32U) == 0U) {
        uint32_t w0 = 0;
        if (length >= 4U) {
            memcpy(&w0, data, sizeof(w0));
        }
        BOOT_PRINTF("[DFU-PATH] usbd_dfu_write blk=%u addr=0x%08lx len=%u w0=0x%08lx\r\n",
                    (unsigned int)block_num,
                    (unsigned long)addr,
                    (unsigned int)length,
                    (unsigned long)w0);
    }

    int write_ret = boot_flash_port_write(addr, data, length);
    if (write_ret != 0) {
        USB_LOG_ERR("[DFU] Write failed at 0x%08lx, len=%u, err=%d\r\n",
                    addr, length, write_ret);
        return write_ret;
    }
    
    return 0;
}

int usbd_dfu_read(uint16_t block_num, const uint8_t *data, uint16_t length, uint16_t *actual_length)
{
    *actual_length = 0;
    return 0;
}

void dfu_check_reset(void)
{
    if (dfu_reset_pending) {
        USB_LOG_INFO("[DFU] Reset requested, flushing data...\r\n");
        BOOT_PRINTF("[DFU] Flushing all pending writes before reset...\r\n");
        
        /* Force fence to ensure all writes are visible */
        fencei();
        
        USB_LOG_INFO("[DFU] Flush complete, resetting in 100ms\r\n");
        board_delay_ms(100);
        extern void boot_port_system_reset(void);
        boot_port_system_reset();
    }
}

/* DFU callback functions required by CherryUSB */
uint8_t *dfu_read_flash(uint8_t *src, uint8_t *dest, uint32_t len)
{
    (void)src;
    memset(dest, 0xFF, len);
    return dest;
}

uint16_t dfu_write_flash(uint8_t *src, uint8_t *dest, uint32_t len)
{
    if (!dfu_erased) {
        int erase_ret = boot_flash_port_erase_app();
        if (erase_ret != 0) {
            USB_LOG_ERR("[DFU] Erase app failed before write: %d\r\n", erase_ret);
            return 0;
        }
        dfu_erased = true;
    }

    dfu_path_accumulate(src, len);
    dfu_rx_blocks++;
    dfu_rx_bytes += len;

    if (!dfu_first_write_logged) {
        uint32_t w0 = 0;
        uint32_t w1 = 0;
        if (len >= 4) {
            memcpy(&w0, src, sizeof(w0));
        }
        if (len >= 8) {
            memcpy(&w1, src + 4, sizeof(w1));
        }
        USB_LOG_INFO("[DFU] First write addr=0x%08lx len=%lu w0=0x%08lx w1=0x%08lx\r\n",
                     (uint32_t)dest,
                     (unsigned long)len,
                     (unsigned long)w0,
                     (unsigned long)w1);
        BOOT_PRINTF("[DFU-PATH] first dfu_write_flash addr=0x%08lx len=%lu w0=0x%08lx w1=0x%08lx\r\n",
                    (unsigned long)dest,
                    (unsigned long)len,
                    (unsigned long)w0,
                    (unsigned long)w1);
        dfu_first_write_logged = true;
    }

    if ((dfu_rx_blocks % 32U) == 0U) {
        uint32_t w0 = 0;
        if (len >= 4U) {
            memcpy(&w0, src, sizeof(w0));
        }
        BOOT_PRINTF("[DFU-PATH] dfu_write_flash blocks=%lu addr=0x%08lx len=%lu w0=0x%08lx checksum=0x%08lx\r\n",
                    (unsigned long)dfu_rx_blocks,
                    (unsigned long)dest,
                    (unsigned long)len,
                    (unsigned long)w0,
                    (unsigned long)dfu_rx_checksum);
    }

    int write_ret = boot_flash_port_write((uint32_t)dest, src, len);
    if (write_ret != 0) {
        USB_LOG_ERR("[DFU] dfu_write_flash failed at 0x%08lx, len=%lu, err=%d\r\n",
                    (uint32_t)dest, len, write_ret);
        return 0;
    }

    return (uint16_t)len;
}

uint16_t dfu_erase_flash(uint32_t addr)
{
    if (addr < boot_flash_port_get_app_start() || addr >= boot_flash_port_get_app_end()) {
        USB_LOG_WRN("[DFU] Ignore erase out-of-range addr: 0x%08lx\r\n", addr);
        return 0;
    }

    int erase_ret = boot_flash_port_erase_sector(addr);
    if (erase_ret != 0) {
        USB_LOG_ERR("[DFU] Erase sector failed at 0x%08lx, err=%d\r\n", addr, erase_ret);
        return 0;
    }

    dfu_erased = true;
    return 1;
}

void dfu_leave(void)
{
    /* Reset to application */
    extern void boot_port_system_reset(void);
    board_delay_ms(100);
    boot_port_system_reset();
}
