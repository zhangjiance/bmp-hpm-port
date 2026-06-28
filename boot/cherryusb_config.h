/*
 * CherryUSB Configuration for HPM5301 DFU Bootloader
 * Based on ethercat_canfd pattern
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "boot_log.h"

/* ================ USB common ================ */
#define CONFIG_USB_PRINTF(...) BOOT_PRINTF(__VA_ARGS__)
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#define CONFIG_USB_ALIGN_SIZE 4
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

/* ================ USB Device ================ */
#define CONFIG_USBDEV_MAX_BUS 1
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#define CONFIG_USBDEV_ADVANCE_DESC
#define CONFIG_USBDEV_EP0_PRIO 4
#define CONFIG_USBDEV_EP0_STACKSIZE 2048

/* DFU */
#define CONFIG_USBDEV_DFU_XFER_SIZE 4096
#define CONFIG_USBDEV_DFU_MAX_BUFSIZE 4096

/* ================ USB Host (required by OTG core) ================ */
#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#define CONFIG_USBHOST_MAX_EHPORTS          4
#define CONFIG_USBHOST_MAX_INTERFACES       4
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 1
#define CONFIG_USBHOST_MAX_ENDPOINTS        4
#define CONFIG_USBHOST_DEV_NAMELEN          32

#endif
