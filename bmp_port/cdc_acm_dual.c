/*
 * Copyright (c) 2022-2023 HPMicro
 * Copyright (c) 2025 BlackMagic Port
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Dual CDC-ACM (GDB + UART/RTT) + DFU Runtime
 * UART/RTT share the same CDC interface (like original BlackMagic)
 */

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usb_dfu.h"
#include "general.h"
#include "gdb_if.h"
#include "hpm_l1c_drv.h"
#include "hpm_uart_drv.h"
#include "rtt.h"
#include "rtt_if.h"
#include "aux_serial.h"
#include "board.h"
#include <string.h>

/* GDB CDC Endpoints (Interface 0-1) */
#define GDB_CDC_IN_EP   0x81
#define GDB_CDC_OUT_EP  0x01
#define GDB_CDC_INT_EP  0x82

/* UART/RTT CDC Endpoints (Interface 2-3) - Shared between UART and RTT */
#define AUX_CDC_IN_EP   0x83
#define AUX_CDC_OUT_EP  0x03
#define AUX_CDC_INT_EP  0x84

/* AUX Serial UART Configuration */
#define AUX_UART        HPM_UART2
#define AUX_UART_IRQ    IRQn_UART2
#define AUX_UART_CLK    clock_uart2

/* RTT Endpoint (Interface 5) - Optional */
#define RTT_IN_EP       0x85

#define DFU_IF_NO  0x04  /* DFU interface number */
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

/* Config descriptor size calculation */
#define GDB_CDC_DESCRIPTOR_LEN  CDC_ACM_DESCRIPTOR_LEN
#define UART_CDC_DESCRIPTOR_LEN CDC_ACM_DESCRIPTOR_LEN
#define DFU_MSOSV2_DESCRIPTOR_LEN (10 + USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_LEN)
#define USB_CONFIG_SIZE (9 + GDB_CDC_DESCRIPTOR_LEN + UART_CDC_DESCRIPTOR_LEN + DFU_DESCRIPTOR_LEN)

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
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x05, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    
    /* GDB CDC Interface (0-1) */
    CDC_ACM_DESCRIPTOR_INIT(0x00, GDB_CDC_INT_EP, GDB_CDC_OUT_EP, GDB_CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    
    /* AUX (UART/RTT) CDC Interface (2-3) */
    CDC_ACM_DESCRIPTOR_INIT(0x02, AUX_CDC_INT_EP, AUX_CDC_OUT_EP, AUX_CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x05),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 4 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x06,                          /* iInterface (String Index 6) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x05, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    
    /* GDB CDC Interface (0-1) */
    CDC_ACM_DESCRIPTOR_INIT(0x00, GDB_CDC_INT_EP, GDB_CDC_OUT_EP, GDB_CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    
    /* AUX (UART/RTT) CDC Interface (2-3) */
    CDC_ACM_DESCRIPTOR_INIT(0x02, AUX_CDC_INT_EP, AUX_CDC_OUT_EP, AUX_CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x05),
    
    /* DFU Runtime Interface Descriptor */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    DFU_IF_NO,                     /* bInterfaceNumber = 4 */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints (Control endpoint only) */
    USB_DEVICE_CLASS_APP_SPECIFIC, /* bInterfaceClass = 0xFE */
    0x01,                          /* bInterfaceSubClass (DFU) */
    0x01,                          /* bInterfaceProtocol (Runtime) */
    0x06,                          /* iInterface (String Index 6) */
    
    /* DFU Functional Descriptor */
    0x09,                          /* bLength */
    0x21,                          /* bDescriptorType (DFU Functional) */
    0x0B,                          /* bmAttributes (bitCanDnload | bitCanUpload | bitManifestationTolerant | bitWillDetach) */
    0xFF, 0x00,                    /* wDetachTimeout = 255 ms */
    0x00, 0x04,                    /* wTransferSize = 1024 bytes */
    0x1A, 0x01                     /* bcdDFUVersion = 1.1a (DfuSe) */
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_1, 0xEF, 0x02, 0x01, 0x01),
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x05, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, GDB_CDC_INT_EP, GDB_CDC_OUT_EP, GDB_CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x02, AUX_CDC_INT_EP, AUX_CDC_OUT_EP, AUX_CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x05),
    
    /* DFU Runtime Interface */
    0x09, USB_DESCRIPTOR_TYPE_INTERFACE, DFU_IF_NO, 0x00, 0x00,
    USB_DEVICE_CLASS_APP_SPECIFIC, 0x01, 0x01, 0x06,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x04, 0x1A, 0x01
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x05, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, GDB_CDC_INT_EP, GDB_CDC_OUT_EP, GDB_CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x02, AUX_CDC_INT_EP, AUX_CDC_OUT_EP, AUX_CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x05),
    
    /* DFU Runtime Interface */
    0x09, USB_DESCRIPTOR_TYPE_INTERFACE, DFU_IF_NO, 0x00, 0x00,
    USB_DEVICE_CLASS_APP_SPECIFIC, 0x01, 0x01, 0x06,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x04, 0x1A, 0x01
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
    "Black Magic Probe (HSlink) v0.2-bmp-hpm-port",  /* Product */
    "2025050401",                 /* Serial Number */
    "Black Magic GDB Server",     /* GDB Interface */
    "Black Magic UART/RTT Port",  /* AUX (UART/RTT) Interface */
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

/* ===== GDB CDC Buffers and Handlers ===== */
#define GDB_RX_BUFFER_SIZE 16384
__attribute__((aligned(64))) uint8_t gdb_usb_write_buffer[GDB_RX_BUFFER_SIZE];
__attribute__((aligned(64))) uint8_t gdb_usb_read_buffer[GDB_RX_BUFFER_SIZE];

volatile bool gdb_usb_tx_busy_flag = false;
volatile uint32_t gdb_usb_tx_count = 0;
volatile uint32_t gdb_usb_rx_count = 0;
volatile uint32_t gdb_usb_rx_offset = 0;

void usbd_cdc_acm_bulk_out_gdb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    l1c_dc_invalidate((uint32_t)gdb_usb_read_buffer, USB_ALIGN_UP(nbytes, 64));
    gdb_usb_rx_count = nbytes;
    gdb_usb_rx_offset = 0;
}

void usbd_cdc_acm_bulk_in_gdb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        usbd_ep_start_write(busid, GDB_CDC_IN_EP, NULL, 0);
    } else {
        gdb_usb_tx_busy_flag = false;
    }
}

struct usbd_endpoint gdb_cdc_out_ep = {
    .ep_addr = GDB_CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out_gdb
};

struct usbd_endpoint gdb_cdc_in_ep = {
    .ep_addr = GDB_CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in_gdb
};

/* ===== AUX (UART/RTT) CDC Buffers and Handlers ===== */
#define AUX_RX_BUFFER_SIZE 4096
#define AUX_UART_TX_BUFFER_SIZE 256
__attribute__((aligned(64))) uint8_t aux_usb_write_buffer[AUX_RX_BUFFER_SIZE];
__attribute__((aligned(64))) uint8_t aux_usb_read_buffer[AUX_RX_BUFFER_SIZE];
__attribute__((aligned(64))) uint8_t aux_uart_tx_buffer[AUX_UART_TX_BUFFER_SIZE];

volatile bool aux_usb_tx_busy_flag = false;
volatile uint32_t aux_usb_tx_count = 0;
volatile uint32_t aux_usb_rx_count = 0;
volatile uint32_t aux_usb_rx_offset = 0;

/* Debug: count USB IN callbacks */
volatile uint32_t aux_usb_in_callback_count = 0;

/* UART RX circular buffer for interrupt-driven reception */
#define AUX_UART_RX_BUFFER_SIZE 1024
static uint8_t aux_uart_rx_buffer[AUX_UART_RX_BUFFER_SIZE];
static volatile uint16_t aux_uart_rx_write_idx = 0;
static volatile uint16_t aux_uart_rx_read_idx = 0;

/* External rtt_enabled flag from rtt.c */
extern bool rtt_enabled;

/* External RTT buffer function from rtt_if.c */
extern uint32_t rtt_write_buffer(const char *buf, uint32_t len);

void usbd_cdc_acm_bulk_out_aux(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    l1c_dc_invalidate((uint32_t)aux_usb_read_buffer, USB_ALIGN_UP(nbytes, 64));
    aux_usb_rx_count = nbytes;
    aux_usb_rx_offset = 0;
    
    /* Data will be processed in main loop:
     * - If RTT enabled: consumed by rtt_getchar() from aux_usb_read_buffer
     * - If RTT disabled: forwarded to UART in aux_serial_uart_poll()
     * 
     * Don't process UART here to avoid blocking USB interrupt context
     */
}

void usbd_cdc_acm_bulk_in_aux(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    
    /* Count callbacks for debugging */
    aux_usb_in_callback_count++;
    
    /* Check if we need to send a zero-length packet for alignment */
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* Send ZLP, keep busy flag set until ZLP completes */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
        /* No ZLP needed (or this IS the ZLP callback), clear busy flag */
        aux_usb_tx_busy_flag = false;
    }
}

struct usbd_endpoint aux_cdc_out_ep = {
    .ep_addr = AUX_CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out_aux
};

struct usbd_endpoint aux_cdc_in_ep = {
    .ep_addr = AUX_CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in_aux
};

/* UART2 Interrupt Handler for RX */
void isr_uart2(void)
{
    uint8_t status = uart_get_irq_id(AUX_UART);
    if (status == uart_intr_id_rx_data_avail || status == uart_intr_id_rx_timeout) {
        while (uart_check_status(AUX_UART, uart_stat_data_ready)) {
            uint8_t ch;
            uart_receive_byte(AUX_UART, &ch);
            
            /* Store in circular buffer */
            uint16_t next_idx = (aux_uart_rx_write_idx + 1) % AUX_UART_RX_BUFFER_SIZE;
            if (next_idx != aux_uart_rx_read_idx) {  /* Not full */
                aux_uart_rx_buffer[aux_uart_rx_write_idx] = ch;
                aux_uart_rx_write_idx = next_idx;
            }
        }
    }
}

/* ===== USB Event Handler ===== */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
    case USBD_EVENT_RESET:
        gdb_usb_tx_busy_flag = false;
        gdb_usb_rx_offset = 0;
        gdb_usb_rx_count = 0;
        gdb_usb_tx_count = 0;
        aux_usb_tx_busy_flag = false;
        aux_usb_rx_offset = 0;
        aux_usb_rx_count = 0;
        aux_usb_tx_count = 0;
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        gdb_usb_tx_busy_flag = false;
        gdb_usb_rx_offset = 0;
        gdb_usb_rx_count = 0;
        gdb_usb_tx_count = 0;
        aux_usb_tx_busy_flag = false;
        aux_usb_rx_offset = 0;
        aux_usb_rx_count = 0;
        aux_usb_tx_count = 0;
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        /* Setup endpoint read transfers */
        usbd_ep_start_read(busid, GDB_CDC_OUT_EP, gdb_usb_read_buffer, GDB_RX_BUFFER_SIZE);
        usbd_ep_start_read(busid, AUX_CDC_OUT_EP, aux_usb_read_buffer, AUX_RX_BUFFER_SIZE);
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;
    default:
        break;
    }
}

/* ===== Interface Instances ===== */
static struct usbd_interface intf_gdb_ctrl;
static struct usbd_interface intf_gdb_data;
static struct usbd_interface intf_aux_ctrl;
static struct usbd_interface intf_aux_data;
static struct usbd_interface intf_dfu;

/* ===== DFU Runtime Interface ===== */
static int dfu_control_request(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    (void)busid;
    
    if (setup->wIndex != DFU_IF_NO)
        return -1;
    
    switch (setup->bRequest) {
    case DFU_REQUEST_GETSTATUS:
        (*data)[0] = DFU_STATUS_OK;
        (*data)[1] = 0;
        (*data)[2] = 0;
        (*data)[3] = 0;
        (*data)[4] = DFU_STATE_APP_IDLE;
        (*data)[5] = 0;
        *len = 6;
        return 0;
        
    case DFU_REQUEST_DETACH:
        extern void platform_request_boot(void);
        platform_request_boot();
        return 0;
        
    case DFU_REQUEST_GETSTATE:
        (*data)[0] = DFU_STATE_APP_IDLE;
        *len = 1;
        return 0;
    }
    
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

/* ===== Initialization ===== */
void cdc_acm_init(uint8_t busid, uint32_t reg_base)
{
    usbd_desc_register(busid, &cdc_descriptor);
    
    /* Add GDB CDC interfaces (0-1) */
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_gdb_ctrl));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_gdb_data));
    usbd_add_endpoint(busid, &gdb_cdc_out_ep);
    usbd_add_endpoint(busid, &gdb_cdc_in_ep);
    
    /* Add AUX (UART/RTT) CDC interfaces (2-3) */
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_aux_ctrl));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_aux_data));
    usbd_add_endpoint(busid, &aux_cdc_out_ep);
    usbd_add_endpoint(busid, &aux_cdc_in_ep);
    
    /* Add DFU Runtime interface (4) */
    usbd_add_interface(busid, dfu_init_intf(&intf_dfu));
    
    usbd_initialize(busid, reg_base, usbd_event_handler);
    
    /* DO NOT initialize UART hardware here - will be done on DTR activation */
    /* aux_serial_init(); */
}

/* ===== DTR Control ===== */
volatile bool gdb_dtr_enable = false;
volatile bool aux_dtr_enable = false;

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    if (intf == 0) {
        gdb_dtr_enable = dtr;
    } else if (intf == 2) {
        aux_dtr_enable = dtr;
        /* Note: UART pins are controlled by debug interface mode (JTAG/SWD) now,
         * not by DTR signal. See jtagtap_init() and swdptap_init() */
    }
}

/* ===== Line Coding ===== */
static struct cdc_line_coding gdb_line_coding = {
    .dwDTERate = 115200,
    .bDataBits = 8,
    .bParityType = 0,
    .bCharFormat = 0,
};

static struct cdc_line_coding aux_line_coding = {
    .dwDTERate = 115200,
    .bDataBits = 8,
    .bParityType = 0,
    .bCharFormat = 0,
};

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    if (intf == 0) {
        memcpy(&gdb_line_coding, line_coding, sizeof(struct cdc_line_coding));
    } else if (intf == 2) {
        memcpy(&aux_line_coding, line_coding, sizeof(struct cdc_line_coding));
        aux_serial_set_encoding(line_coding);
    }
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    if (intf == 0) {
        memcpy(line_coding, &gdb_line_coding, sizeof(struct cdc_line_coding));
    } else if (intf == 2) {
        memcpy(line_coding, &aux_line_coding, sizeof(struct cdc_line_coding));
    }
}

/* ===== GDB Interface API ===== */
char *gdb_if_get_buffer(void)
{
    return (char *)gdb_usb_write_buffer;
}

uint32_t gdb_if_get_available_space(void)
{
    return gdb_usb_tx_busy_flag ? 0 : GDB_RX_BUFFER_SIZE;
}

void gdb_if_putchar(char ch, bool flush)
{
    /* Wait for previous transmission to complete */
    while (gdb_usb_tx_busy_flag);
    
    gdb_usb_write_buffer[gdb_usb_tx_count++] = ch;
    
    if (flush || gdb_usb_tx_count >= GDB_RX_BUFFER_SIZE) {
        l1c_dc_flush((uint32_t)gdb_usb_write_buffer, USB_ALIGN_UP(gdb_usb_tx_count, 64));
        gdb_usb_tx_busy_flag = true;
        usbd_ep_start_write(0, GDB_CDC_IN_EP, gdb_usb_write_buffer, gdb_usb_tx_count);
        gdb_usb_tx_count = 0;
    }
}

bool gdb_serial_get_dtr(void)
{
    return gdb_dtr_enable;
}

char gdb_if_getchar(void)
{
    while (gdb_usb_rx_count == 0);
    
    char c = gdb_usb_read_buffer[gdb_usb_rx_offset++];
    
    if (gdb_usb_rx_offset >= gdb_usb_rx_count) {
        gdb_usb_rx_count = 0;
        gdb_usb_rx_offset = 0;
        usbd_ep_start_read(0, GDB_CDC_OUT_EP, gdb_usb_read_buffer, GDB_RX_BUFFER_SIZE);
    }
    
    return c;
}

char gdb_if_getchar_to(const uint32_t timeout)
{
    const uint32_t start = platform_time_ms();
    
    while (gdb_usb_rx_count == 0 && (platform_time_ms() - start) < timeout);
    
    if (gdb_usb_rx_count == 0)
        return -1;
    
    return gdb_if_getchar();
}

/* ===== AUX (UART/RTT) Interface API ===== */
char *aux_serial_current_transmit_buffer(void)
{
    return (char *)aux_usb_write_buffer + aux_usb_tx_count;
}

size_t aux_serial_transmit_buffer_fullness(void)
{
    return aux_usb_tx_count;
}

void aux_serial_usb_send(size_t len)
{
    if (aux_usb_tx_busy_flag || len == 0)
        return;
    
    l1c_dc_flush((uint32_t)aux_usb_write_buffer, USB_ALIGN_UP(len, 64));
    aux_usb_tx_busy_flag = true;
    usbd_ep_start_write(0, AUX_CDC_IN_EP, aux_usb_write_buffer, len);
    aux_usb_tx_count = 0;
}

void aux_serial_restart_rx(void)
{
    aux_usb_rx_count = 0;
    aux_usb_rx_offset = 0;
    usbd_ep_start_read(0, AUX_CDC_OUT_EP, aux_usb_read_buffer, AUX_RX_BUFFER_SIZE);
}

void aux_serial_send(size_t len)
{
    aux_usb_tx_count = len;
    aux_serial_usb_send(len);
}

/* Get character from aux serial RX buffer (UART only, RTT has its own path) */
int32_t aux_serial_getchar(void)
{
    /* Check UART RX circular buffer */
    if (aux_uart_rx_read_idx == aux_uart_rx_write_idx)
        return -1;
    
    char c = aux_uart_rx_buffer[aux_uart_rx_read_idx];
    aux_uart_rx_read_idx = (aux_uart_rx_read_idx + 1) % AUX_UART_RX_BUFFER_SIZE;
    return (int32_t)(uint8_t)c;
}

/* Stub implementations for compatibility */
void aux_serial_update_receive_buffer_fullness(void) {}
bool aux_serial_receive_buffer_empty(void) { 
    return (aux_uart_rx_read_idx == aux_uart_rx_write_idx);
}
void aux_serial_drain_receive_buffer(void) { 
    aux_uart_rx_read_idx = aux_uart_rx_write_idx;
}
void aux_serial_stage_receive_buffer(void) {}

/* RTT functions are implemented in rtt_if.c */
/* UART polling task - called from main loop */
void aux_serial_uart_poll(void)
{
#ifdef ENABLE_RTT
    if (rtt_enabled) {
        /* RTT Mode: Forward USB RX data to RTT down buffer (host→target) */
        /* Note: RTT TX (target→host) is handled directly in rtt_write() */
        
        if (aux_usb_rx_count > 0) {
            uint32_t written = rtt_write_buffer((const char *)aux_usb_read_buffer, aux_usb_rx_count);
            (void)written; /* Ignore if buffer full, data will be dropped */
            
            /* Restart USB RX */
            aux_usb_rx_count = 0;
            aux_usb_rx_offset = 0;
            usbd_ep_start_read(0, AUX_CDC_OUT_EP, aux_usb_read_buffer, AUX_RX_BUFFER_SIZE);
        }
        
        return;
    }
#endif
    
    /* UART Mode: Forward data between USB and UART */
    
    /* Skip if UART not initialized yet */
    extern bool aux_serial_pins_enabled(void);
    if (!aux_serial_pins_enabled())
        return;
    
    /* 1. Forward USB RX data to UART TX (USB→UART) */
    if (aux_usb_rx_count > 0) {
        for (uint32_t i = 0; i < aux_usb_rx_count; i++) {
            /* Non-blocking send - skip if FIFO full */
            if (uart_check_status(AUX_UART, uart_stat_transmitter_empty)) {
                uart_write_byte(AUX_UART, aux_usb_read_buffer[i]);
            }
        }
        /* Mark data as consumed and restart USB RX */
        aux_usb_rx_count = 0;
        aux_usb_rx_offset = 0;
        usbd_ep_start_read(0, AUX_CDC_OUT_EP, aux_usb_read_buffer, AUX_RX_BUFFER_SIZE);
    }
    
    /* 2. Send UART RX data to USB (UART→USB) */
    if (aux_usb_tx_busy_flag)
        return;
    
    uint16_t available = 0;
    if (aux_uart_rx_write_idx >= aux_uart_rx_read_idx)
        available = aux_uart_rx_write_idx - aux_uart_rx_read_idx;
    else
        available = AUX_UART_RX_BUFFER_SIZE - aux_uart_rx_read_idx + aux_uart_rx_write_idx;
    
    if (available > 0) {
        uint16_t to_send = (available > AUX_RX_BUFFER_SIZE) ? AUX_RX_BUFFER_SIZE : available;
        
        for (uint16_t i = 0; i < to_send; i++) {
            aux_usb_write_buffer[i] = aux_uart_rx_buffer[aux_uart_rx_read_idx];
            aux_uart_rx_read_idx = (aux_uart_rx_read_idx + 1) % AUX_UART_RX_BUFFER_SIZE;
        }
        
        aux_serial_usb_send(to_send);
    }
}
