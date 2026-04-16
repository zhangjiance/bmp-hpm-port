/*
 * Simple DFU-Only USB Descriptor for HPM Bootloader
 * 
 * Minimalist implementation to get USB enumeration working
 */
#include "usbd_core.h"
#include "usb_dfu.h"
#include "usbd_dfu.h"
#include "board.h"
#include "boot_log.h"
#include <string.h>

/* Port layer interface for flash operations */
#include "port/boot_flash_port.h"

#define USBD_VID           0x34B7  /* HPMicro VID */
#define USBD_PID           0x1002  /* DFU Device PID */
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define DFU_INTERFACE_NUMBER 0x00

/* Total config descriptor size: Config(9) + Interface(9) + DFU Functional(9) */
#define USB_CONFIG_SIZE (9 + 9 + 9)

/* ========================================================================
 * USB Device Descriptor
 * ======================================================================== */
static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01)
};

/* ========================================================================
 * Configuration Descriptor (DFU Only)
 * ======================================================================== */
static const uint8_t config_descriptor[] = {
    /* Configuration Descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    
    /* DFU Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_INTERFACE_NUMBER,          /* bInterfaceNumber = 0 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (DFU uses control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass: 0xFE */
    0x01,                          /* bInterfaceSubClass: DFU */
    0x02,                          /* bInterfaceProtocol: DFU mode */
    0x02,                          /* iInterface (String Index 2) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes: 
                                    * bit 0: bitCanDnload = 1 (can download)
                                    * bit 1: bitCanUpload = 1 (can upload)
                                    * bit 2: bitManifestationTolerant = 0
                                    * bit 3: bitWillDetach = 1 (will detach)
                                    */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a */
};

/* ========================================================================
 * String Descriptors
 * ======================================================================== */
static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },  /* Langid: English US */
    "HPMicro",                      /* Manufacturer */
    "HPM DFU Bootloader",           /* Product */
    "DFU001",                       /* Serial Number */
};

/* ========================================================================
 * Descriptor Callbacks
 * ======================================================================== */
static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(string_descriptors[0]))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor simple_dfu_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = NULL,
    .other_speed_descriptor_callback = NULL,
    .string_descriptor_callback = string_descriptor_callback,
    .bos_descriptor = NULL,
    .msosv2_descriptor = NULL,
};

/* ========================================================================
 * DFU Flash Operation Callbacks
 * ======================================================================== */

/**
 * @brief DFU leave/detach callback
 */
void dfu_leave(void)
{
    BOOT_PRINTF("[DFU] Leave request, resetting device...\r\n");
    board_delay_ms(100);
    
    /* Perform system reset */
    extern void boot_port_system_reset(void);
    boot_port_system_reset();
}

/**
 * @brief DFU erase flash
 * @param add Flash address to erase
 * @return 0 on success
 */
uint16_t dfu_erase_flash(uint32_t add)
{
    BOOT_PRINTF("[DFU] Erase at 0x%08lx (discard mode)\r\n", add);
    return 0;
}

/**
 * @brief DFU write flash
 * @param src Source buffer
 * @param dest Destination flash address
 * @param len Data length
 * @return 0 on success
 */
uint16_t dfu_write_flash(uint8_t *src, uint8_t *dest, uint32_t len)
{
    (void)src;
    (void)dest;
    return (uint16_t)len;
}

/**
 * @brief DFU read flash
 * @param src Source flash address
 * @param dest Destination buffer
 * @param len Data length
 * @return dest pointer
 */
uint8_t *dfu_read_flash(uint8_t *src, uint8_t *dest, uint32_t len)
{
    (void)src;
    memset(dest, 0xFF, len);
    return dest;
}

/* ========================================================================
 * DFU Initialization
 * ======================================================================== */

static struct usbd_interface intf0;

static void usbd_event_handler(uint8_t busid, uint8_t event);

/**
 * @brief Initialize simple DFU bootloader
 * @param busid USB bus ID
 * @param reg_base USB controller base address
 */
void simple_dfu_init(uint8_t busid, uintptr_t reg_base)
{
    BOOT_PRINTF("[DFU] Initializing simple DFU bootloader...\r\n");
    
    /* Register USB descriptors */
    usbd_desc_register(busid, &simple_dfu_descriptor);
    
    /* Initialize DFU interface */
    usbd_add_interface(busid, usbd_dfu_init_intf(&intf0));
    
    /* Initialize USB device */
    usbd_initialize(busid, reg_base, usbd_event_handler);
    
    BOOT_PRINTF("[DFU] USB DFU initialized successfully\r\n");
}

/* ========================================================================
 * USB Event Handler
 * ======================================================================== */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    
    switch (event) {
        case USBD_EVENT_RESET:
            BOOT_PRINTF("[USB] Reset\r\n");
            break;
        case USBD_EVENT_CONFIGURED:
            BOOT_PRINTF("[USB] Configured\r\n");
            break;
        case USBD_EVENT_SUSPEND:
            BOOT_PRINTF("[USB] Suspend\r\n");
            break;
        case USBD_EVENT_RESUME:
            BOOT_PRINTF("[USB] Resume\r\n");
            break;
        default:
            break;
    }
}
