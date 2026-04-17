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
    if (uart_pins_configured)
        return;
    
    /* Initialize UART2 hardware when user opens the port */
    uart_config_t config = {0};
    
    /* Configure pins first */
    board_init_uart(AUX_UART);
    
    /* Configure UART */
    uart_default_config(AUX_UART, &config);
    config.baudrate = aux_line_coding.dwDTERate;
    config.num_of_stop_bits = (aux_line_coding.bCharFormat == 0) ? stop_bits_1 : stop_bits_2;
    config.word_length = (aux_line_coding.bDataBits == 8) ? word_length_8_bits : word_length_7_bits;
    
    switch (aux_line_coding.bParityType) {
        case 0:  config.parity = parity_none; break;
        case 1:  config.parity = parity_odd; break;
        case 2:  config.parity = parity_even; break;
        default: config.parity = parity_none; break;
    }
    
    config.fifo_enable = true;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    
    (void)board_init_uart_clock(AUX_UART);
    if (status_success != uart_init(AUX_UART, &config)) {
        return;
    }
    
    /* Enable RX interrupt */
    uart_enable_irq(AUX_UART, uart_intr_rx_data_avail_or_timeout);
    intc_m_enable_irq_with_priority(AUX_UART_IRQ, 2);
    
    uart_pins_configured = true;
}

/* Disable UART2 pins - called when switching to JTAG mode */
void aux_serial_disable_pins(void)
{
    if (!uart_pins_configured)
        return;
    
    /* Disable UART interrupt */
    intc_m_disable_irq(AUX_UART_IRQ);
    uart_disable_irq(AUX_UART, uart_intr_rx_data_avail_or_timeout);
    
    /* Call board-level pin uninit */
    extern void uninit_uart2_pins(void);
    uninit_uart2_pins();
    
    uart_pins_configured = false;
}

bool aux_serial_pins_enabled(void)
{
    return uart_pins_configured;
}

void aux_serial_set_encoding(const struct cdc_line_coding *coding)
{
    if (!coding)
        return;
    
    /* Save configuration */
    memcpy(&aux_line_coding, coding, sizeof(struct cdc_line_coding));
    
    /* Reconfigure UART if already initialized */
    if (!uart_pins_configured)
        return;
    
    uart_config_t config = {0};
    uart_default_config(AUX_UART, &config);
    config.baudrate = coding->dwDTERate;
    config.num_of_stop_bits = (coding->bCharFormat == 0) ? stop_bits_1 : stop_bits_2;
    config.word_length = (coding->bDataBits == 8) ? word_length_8_bits : word_length_7_bits;
    
    switch (coding->bParityType) {
        case 0:  config.parity = parity_none; break;
        case 1:  config.parity = parity_odd; break;
        case 2:  config.parity = parity_even; break;
        default: config.parity = parity_none; break;
    }
    
    config.fifo_enable = true;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    
    uart_init(AUX_UART, &config);
    uart_enable_irq(AUX_UART, uart_intr_rx_data_avail_or_timeout);
}

void aux_serial_get_encoding(struct cdc_line_coding *coding)
{
    if (coding) {
        memcpy(coding, &aux_line_coding, sizeof(struct cdc_line_coding));
    }
}
