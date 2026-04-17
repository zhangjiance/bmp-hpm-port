/*
 * Copyright (c) 2025 BlackMagic Port for HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RTT (Real-Time Transfer) interface implementation
 * This forwards RTT data from target to USB directly (no intermediate buffering)
 */

#include "general.h"
#include "rtt.h"
#include "rtt_if.h"
#include "platform.h"
#include "aux_serial.h"
#include <string.h>

/* Debug: track RTT data flow */
static uint32_t rtt_write_total = 0;

/* RTT down buffer (host to target) */
static char rtt_down_buffer[RTT_DOWN_BUF_SIZE];
static uint32_t rtt_down_read_index = 0;
static uint32_t rtt_down_write_index = 0;

int rtt_if_init(void)
{
    /* Initialize RTT interface */
    rtt_down_read_index = 0;
    rtt_down_write_index = 0;
    
    return 0;
}

int rtt_if_exit(void)
{
    /* Cleanup RTT interface */
    return 0;
}

/* External USB busy flag from cdc_acm_dual.c */
extern volatile bool aux_usb_tx_busy_flag;

/* Write len bytes from target to host (USB) - Direct USB transmission */
uint32_t rtt_write(const uint32_t channel, const char *buf, uint32_t len)
{
    /* Accept all channels - merge into one USB stream */
    if (!buf || len == 0)
        return 0;
    
    /* Check if USB is busy - if so, drop this packet */
    /* CRITICAL: Must check aux_usb_tx_busy_flag, not aux_serial_transmit_buffer_fullness() */
    /* because aux_usb_tx_count is set to 0 when transmission starts */
    if (aux_usb_tx_busy_flag)
        return 0;  /* USB busy, data will be dropped */
    
    /* Get USB transmit buffer */
    char *usb_buf = aux_serial_current_transmit_buffer();
    
    /* Limit to buffer size (4KB) */
    uint32_t to_send = (len > 4096U) ? 4096U : len;
    
    /* Copy data directly to USB buffer */
    memcpy(usb_buf, buf, to_send);
    
    /* Send via USB */
    aux_serial_send(to_send);
    
    rtt_write_total += to_send;
    
    return to_send;
}

/* Read one character from host to target (USB to target) */
int32_t rtt_getchar(const uint32_t channel)
{
    /* We only support one down channel merged from USB */
    (void)channel;
    
    /* Check if data available */
    if (rtt_down_read_index == rtt_down_write_index)
        return -1;
    
    char ch = rtt_down_buffer[rtt_down_read_index];
    rtt_down_read_index = (rtt_down_read_index + 1) % RTT_DOWN_BUF_SIZE;
    
    return (int32_t)(uint8_t)ch;
}

/* Check if no data available for reading */
bool rtt_nodata(const uint32_t channel)
{
    /* We only support one down channel */
    (void)channel;
    
    return (rtt_down_read_index == rtt_down_write_index);
}

/* Write to RTT down buffer from USB */
uint32_t rtt_write_buffer(const char *buf, uint32_t len)
{
    if (!buf || len == 0)
        return 0;
    
    uint32_t written = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        uint32_t next_index = (rtt_down_write_index + 1) % RTT_DOWN_BUF_SIZE;
        
        /* Check if buffer is full */
        if (next_index == rtt_down_read_index)
            break;
        
        rtt_down_buffer[rtt_down_write_index] = buf[i];
        rtt_down_write_index = next_index;
        written++;
    }
    
    return written;
}

/* Debug function to track data flow */
uint32_t rtt_get_write_total(void)
{
    return rtt_write_total;
}
