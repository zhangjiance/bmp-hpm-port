/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usb_dfu.h"
#include "general.h"
#include "gdb_if.h"
#include "hpm_l1c_drv.h"

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x83

#define DFU_IF_NO  0x02  /* DFU interface number */
#define WINUSB_VENDOR_CODE 0x20

#define CDC_MAX_PACKET_SIZE 512

#ifdef CONFIG_USB_HS
#if CDC_MAX_PACKET_SIZE != 512
#error "CDC_MAX_PACKET_SIZE must be 512 in hs"
#endif
#else
#if CDC_MAX_PACKET_SIZE != 64
#error "CDC_MAX_PACKET_SIZE must be 64 in fs"
#endif
#endif

/*!< config descriptor size */
#define DFU_DESCRIPTOR_LEN (9 + 9)  /* Interface + Functional descriptor */
#define DFU_MSOSV2_DESCRIPTOR_LEN (10 + USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_LEN)
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN + DFU_DESCRIPTOR_LEN)

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t dfu_winusb_msosv2_desc_set[] = {
    USB_MSOSV2_COMP_ID_SET_HEADER_DESCRIPTOR_INIT(DFU_MSOSV2_DESCRIPTOR_LEN),
    USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_INIT(DFU_IF_NO),
};

static const struct usb_msosv2_descriptor msosv2_descriptor = {
    .vendor_code = WINUSB_VENDOR_CODE,
    .compat_id = dfu_winusb_msosv2_desc_set,
    .compat_id_len = sizeof(dfu_winusb_msosv2_desc_set),
};

static const uint8_t config_descriptor_hs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 2 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x04,                          /* iInterface (String Index 4) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 2 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x04,                          /* iInterface (String Index 4) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, 0x01),
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 2 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x04,                          /* iInterface (String Index 4) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 2 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x04,                          /* iInterface (String Index 4) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
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
    (const char[]){ 0x09, 0x04 }, /* Langid */
    "HPMicro",                    /* Manufacturer */
    "Black Magic Probe (HSlink) v0.2-bmp-hpm-port) ",           /* Product */
    "2025050401",                 /* Serial Number */
    "Black Magic Firmware Upgrade", /* DFU Interface */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return config_descriptor_hs;
    } else if (speed == USB_SPEED_FULL) {
        return config_descriptor_fs;
    } else {
        return NULL;
    }
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_quality_descriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return other_speed_config_descriptor_hs;
    } else if (speed == USB_SPEED_FULL) {
        return other_speed_config_descriptor_fs;
    } else {
        return NULL;
    }
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .msosv2_descriptor = &msosv2_descriptor,
    .bos_descriptor = &bos_descriptor,
    .string_descriptor_callback = string_descriptor_callback,
};

#define USB_RX_BUFFER_SIZE 16384
__attribute__((aligned(64))) uint8_t g_usb_write_buffer[USB_RX_BUFFER_SIZE];
__attribute__((aligned(64))) uint8_t g_usb_read_buffer[USB_RX_BUFFER_SIZE];

volatile bool g_usb_tx_busy_flag = false;
volatile uint32_t g_usb_tx_count = 0;
volatile uint32_t g_usb_rx_count = 0;
volatile uint32_t g_usb_rx_offset = 0;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
    case USBD_EVENT_RESET:
        g_usb_tx_busy_flag = false;
        g_usb_rx_offset = 0;
        g_usb_rx_count = 0;
        g_usb_tx_count = 0;
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        g_usb_tx_busy_flag = false;
        g_usb_rx_offset = 0;
        g_usb_rx_count = 0;
        g_usb_tx_count = 0;
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        /* setup first out ep read transfer */
        usbd_ep_start_read(busid, CDC_OUT_EP, g_usb_read_buffer, USB_RX_BUFFER_SIZE);
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;

    default:
        break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;

    l1c_dc_invalidate((uint32_t)g_usb_read_buffer, USB_ALIGN_UP(nbytes, 64));
    g_usb_rx_count = nbytes;
    g_usb_rx_offset = 0;
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;

    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    } else {
        g_usb_tx_busy_flag = false;
    }
}

/*!< endpoint call back */
struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

/* DFU Runtime Interface */
static struct usbd_interface intf_dfu;

/* DFU Control Request Handler */
static int dfu_control_request(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    (void)busid;
    
    /* Check if request is for DFU interface */
    if (setup->wIndex != DFU_IF_NO)
        return -1;  /* Not for DFU interface */
    
    switch (setup->bRequest) {
    case DFU_REQUEST_GETSTATUS:
        (*data)[0] = DFU_STATUS_OK;
        (*data)[1] = 0;
        (*data)[2] = 0;
        (*data)[3] = 0;
        (*data)[4] = DFU_STATE_APP_IDLE;  /* DFU state */
        (*data)[5] = 0;  /* iString not used */
        *len = 6;
        return 0;
        
    case DFU_REQUEST_DETACH:
        /* Jump to bootloader after status stage completes */
        /* Note: CherryUSB will complete the status stage before we reset */
        extern void platform_request_boot(void);
        platform_request_boot();
        /* Never returns - system resets and enters bootloader */
        return 0;
        
    case DFU_REQUEST_GETSTATE:
        (*data)[0] = DFU_STATE_APP_IDLE;
        *len = 1;
        return 0;
    }
    
    /* Unsupported request */
    return -1;
}

static void dfu_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)busid;
    (void)event;
    (void)arg;
}

static struct usbd_interface *dfu_init_intf(struct usbd_interface *intf)
{
    intf->class_interface_handler = dfu_control_request;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = dfu_notify_handler;
    
    return intf;
}

/* function ------------------------------------------------------------------*/

void cdc_acm_init(uint8_t busid, uint32_t reg_base)
{
    usbd_desc_register(busid, &cdc_descriptor);
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
    usbd_add_interface(busid, dfu_init_intf(&intf_dfu));  /* Add DFU Runtime interface */
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);
    usbd_initialize(busid, reg_base, usbd_event_handler);
}

volatile bool dtr_enable = false;

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    if (dtr) {
        //printf("remote attach \r\n");
    } else {
        //printf("remote detach \r\n");
    }

    dtr_enable = dtr;
}

void gdb_if_putchar(const char c, const bool flush)
{
    g_usb_write_buffer[g_usb_tx_count++] = c;

    if(flush)
    {
        g_usb_tx_busy_flag = true;
        l1c_dc_writeback((uint32_t)g_usb_write_buffer, USB_ALIGN_UP(g_usb_tx_count, 64));
        usbd_ep_start_write(0, CDC_IN_EP, (uint8_t *)core_local_mem_to_sys_address(0, (uint32_t)g_usb_write_buffer), g_usb_tx_count);
        while (g_usb_tx_busy_flag) {
        }
        g_usb_tx_count = 0;
    }
}

void gdb_if_flush(const bool force)
{
    g_usb_tx_busy_flag = true;
    l1c_dc_writeback((uint32_t)g_usb_write_buffer, USB_ALIGN_UP(g_usb_tx_count, 64));
    usbd_ep_start_write(0, CDC_IN_EP, (uint8_t *)core_local_mem_to_sys_address(0, (uint32_t)g_usb_write_buffer), g_usb_tx_count);
    while (g_usb_tx_busy_flag) {
    }
    g_usb_tx_count = 0;
}

static int __gdb_if_getchar(void)
{
    if (dtr_enable == false) {
        return '\04';
    }

    if (g_usb_rx_count > 0) {
        if (g_usb_rx_offset < g_usb_rx_count) {
            return g_usb_read_buffer[g_usb_rx_offset++];
        } else {
            g_usb_rx_count = 0;
            /* setup first out ep read transfer */
            usbd_ep_start_read(0, CDC_OUT_EP, g_usb_read_buffer, USB_RX_BUFFER_SIZE);
            return -1;
        }
    } else {
        return -1;
    }
}

char gdb_if_getchar(void)
{
    int c;

    while((c = __gdb_if_getchar()) == -1)
    {

    }

    return (char)c;
}

char gdb_if_getchar_to(const uint32_t timeout)
{
	int c = 0;
	platform_timeout_s receive_timeout;
	platform_timeout_set(&receive_timeout, timeout);

	/* Wait while we need more data or until the timeout expires */
	while (!platform_timeout_is_expired(&receive_timeout))
	{
       c = __gdb_if_getchar();
       if(c != -1)
       {
           return (char)c;
       }
	}
	return -1;
}