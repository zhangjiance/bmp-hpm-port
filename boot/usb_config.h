/*
 * USB Configuration for UF2 Bootloader
 */

#ifndef USB_CONFIG_H
#define USB_CONFIG_H

/* USB printf configuration */
#ifndef CONFIG_USB_PRINTF
#define CONFIG_USB_PRINTF(...) ((void)0)
#endif

/* USB Device Configuration */
#define CONFIG_USBDEV_MAX_BUS 1
#define CONFIG_USBDEV_ADVANCE_DESC 1  /* Use advanced descriptor API */

/* Endpoint Configuration */
#define CONFIG_USBDEV_EP_NUM 8

/* USB Transfer Buffer Configuration */
#define CONFIG_USB_ALIGN_SIZE 4
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512

/* USB Host Configuration (needed for core compilation) */
#define CONFIG_USBHOST_MAX_ENDPOINTS 4
#define CONFIG_USBHOST_DEV_NAMELEN 32
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 1
#define CONFIG_USBHOST_MAX_INTERFACES 6
#define CONFIG_USBHOST_MAX_EHPORTS 4

/* Enable USB Device MSC Class */
#define CONFIG_USBDEV_MSC_BLOCK_SIZE 512
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 4096
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "HPMicro"
#define CONFIG_USBDEV_MSC_PRODUCT_STRING      "UF2 Bootloader"
#define CONFIG_USBDEV_MSC_VERSION_STRING      "1.0"

/* Enable USB Device DFU Class */
#define CONFIG_USBDEV_DFU_TRANSFER_SIZE 512

/* Logging Configuration */
#ifndef USBD_LOG_LEVEL
#define USBD_LOG_LEVEL USB_LOG_INFO
#endif

/* USB Memory attributes */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))
#define USB_MEM_ALIGNX __attribute__((aligned(64)))

/* CherryUSB Feature Configuration */
#define USBD_IRQ_HANDLER USB0_IRQHandler
#define USBD_NUM 1

#ifndef CONFIG_HPM_USBD_BASE
#define CONFIG_HPM_USBD_BASE HPM_USB0_BASE
#endif

#ifndef CONFIG_HPM_USBD_IRQn
#define CONFIG_HPM_USBD_IRQn IRQn_USB0
#endif

#endif /* USB_CONFIG_H */
