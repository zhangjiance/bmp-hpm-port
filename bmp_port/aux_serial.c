/*
 * Copyright (c) 2025 BlackMagic Port for HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * AUX Serial (UART2) implementation
 * Implements target board UART pass-through on PA08/PA09
 */

#include "general.h"
#include "platform.h"
#include "aux_serial.h"
#include "board.h"
#include "hpm_uart_drv.h"
#include "hpm_l1c_drv.h"
#include <string.h>

/* Include CDC line coding definition */
struct cdc_line_coding {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
} __attribute__((packed));

#define AUX_UART        HPM_UART2
#define AUX_UART_IRQ    IRQn_UART2
#define AUX_UART_CLK    clock_uart2

/* External references to AUX CDC buffers from cdc_acm_dual.c */
extern uint8_t aux_usb_write_buffer[];
extern volatile uint32_t aux_usb_tx_count;
extern volatile uint32_t aux_usb_rx_count;
extern volatile uint32_t aux_usb_rx_offset;
extern volatile bool aux_usb_tx_busy_flag;
extern volatile bool aux_rtt_mode;

/* Line coding storage */
static struct cdc_line_coding aux_line_coding = {
    .dwDTERate = 115200,
    .bDataBits = 8,
    .bParityType = 0,
    .bCharFormat = 0,
};

/* Track if UART pins are configured (to avoid conflict with JTAG TDI on PA08) */
static bool uart_pins_configured = false;

void aux_serial_init(void)
{
    /* COMPLETELY DISABLED - UART initialization conflicts with JTAG TDI on PA08
     * AUX CDC will be stub only, discard all data */
    
    /* Do nothing - no UART hardware initialization */
}

void aux_serial_enable_pins(void)
{
    /* STUB - do nothing, UART hardware completely disabled */
}

bool aux_serial_pins_enabled(void)
{
    return uart_pins_configured;
}

void aux_serial_set_encoding(const struct cdc_line_coding *coding)
{
    if (!coding)
        return;
    
    /* STUB - Only save configuration, do not configure hardware */
    memcpy(&aux_line_coding, coding, sizeof(struct cdc_line_coding));
}

void aux_serial_get_encoding(struct cdc_line_coding *coding)
{
    if (coding) {
        memcpy(coding, &aux_line_coding, sizeof(struct cdc_line_coding));
    }
}
