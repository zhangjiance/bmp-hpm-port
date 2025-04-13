/*********************************************************************************
  *Copyright (C), 2016-2025, jiance.zhang
  *FileName:  cdc_acm.h
  *Author:  JianCe.Zhang
  *Version:  V1.0
  *Date: 2025-04-06
  *Description: 
  *Function List:
  * 1. void function(void)
  * 2. 
  * 3. 
  * 4. 
  *History:
  * 1. 2025-04-06;JianCe.Zhang;Init Function 
  * 2. 
  * 3. 
  * 4. 
**********************************************************************************/

#ifndef  __cdc_acm_H__
#define  __cdc_acm_H__

#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_msc.h"
#include "chry_ringbuffer.h"

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x83

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

#define USBD_WINUSB_VENDOR_CODE 0x20

#define USBD_WEBUSB_ENABLE 0
#define USBD_BULK_ENABLE   1
#define USBD_WINUSB_ENABLE 1

/* WinUSB Microsoft OS 2.0 descriptor sizes */
#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE  10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE 8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE  20

#define FUNCTION_SUBSET_LEN                160
#define DEVICE_INTERFACE_GUIDS_FEATURE_LEN 132

#define USBD_WINUSB_DESC_SET_LEN (WINUSB_DESCRIPTOR_SET_HEADER_SIZE + USBD_WEBUSB_ENABLE * FUNCTION_SUBSET_LEN + USBD_BULK_ENABLE * FUNCTION_SUBSET_LEN)

#define USBD_NUM_DEV_CAPABILITIES (USBD_WEBUSB_ENABLE + USBD_WINUSB_ENABLE)

#define USBD_WEBUSB_DESC_LEN 24
#define USBD_WINUSB_DESC_LEN 28

#define USBD_BOS_WTOTALLENGTH (0x05 +                                      \
                               USBD_WEBUSB_DESC_LEN * USBD_WEBUSB_ENABLE + \
                               USBD_WINUSB_DESC_LEN * USBD_WINUSB_ENABLE)

#define CONFIG_GDBOUT_RINGBUF_SIZE (8 * 1024)
#define CONFIG_USBRX_RINGBUF_SIZE  (8 * 1024)

#ifdef __cplusplus
extern "C"
{
#endif

extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t gdbout_ringbuffer[CONFIG_GDBOUT_RINGBUF_SIZE];
extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usbrx_ringbuffer[CONFIG_USBRX_RINGBUF_SIZE];
extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usb_tmpbuffer[CDC_MAX_PACKET_SIZE];

// extern const struct usb_descriptor cmsisdap_descriptor;
// extern __ALIGN_BEGIN const uint8_t USBD_WinUSBDescriptorSetDescriptor[];
// extern __ALIGN_BEGIN const uint8_t USBD_BinaryObjectStoreDescriptor[];
// extern char *string_descriptors[];

// extern struct usbd_interface dap_intf;
// extern struct usbd_interface intf1;
// extern struct usbd_interface intf2;
// extern struct usbd_interface intf3;
// extern struct usbd_interface hid_intf;

// extern struct usbd_endpoint dap_out_ep;
// extern struct usbd_endpoint dap_in_ep;
// extern struct usbd_endpoint cdc_out_ep;
// extern struct usbd_endpoint cdc_in_ep;

extern USB_NOCACHE_RAM_SECTION chry_ringbuffer_t g_usbrx;
extern USB_NOCACHE_RAM_SECTION chry_ringbuffer_t g_gdbout;

extern USB_NOCACHE_RAM_SECTION volatile uint8_t usbrx_idle_flag;
extern USB_NOCACHE_RAM_SECTION volatile uint8_t usbtx_idle_flag;


void chry_dap_handle(void);

void chry_dap_usb2uart_handle(void);

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding);

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len);

void chry_dap_usb2uart_uart_send_complete(uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
/* [] END OF cdc_acm.h */