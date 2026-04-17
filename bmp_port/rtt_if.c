/*
 * Copyright (c) 2025 BlackMagic Port for HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RTT (Real-Time Transfer) interface implementation
 * This forwards RTT data from target to USB
 */

#include "general.h"
#include "rtt.h"
#include "rtt_if.h"
#include "platform.h"

/* Debug: track RTT data flow */
static uint32_t rtt_write_total = 0;
static uint32_t rtt_read_total = 0;

/* RTT transfer buffers */
static char rtt_up_buffer[RTT_UP_BUF_SIZE];
static char rtt_down_buffer[RTT_DOWN_BUF_SIZE];

static uint32_t rtt_up_read_index = 0;
static uint32_t rtt_up_write_index = 0;
static uint32_t rtt_down_read_index = 0;
static uint32_t rtt_down_write_index = 0;

int rtt_if_init(void)
{
    /* Initialize RTT interface */
    rtt_up_read_index = 0;
    rtt_up_write_index = 0;
    rtt_down_read_index = 0;
    rtt_down_write_index = 0;
    
    return 0;
}

int rtt_if_exit(void)
{
    /* Cleanup RTT interface */
    return 0;
}

/* Write len bytes from target to host (USB) */
uint32_t rtt_write(const uint32_t channel, const char *buf, uint32_t len)
{
    /* Accept all channels - we merge them into one USB stream */
    if (!buf || len == 0)
        return 0;
    
    uint32_t written = 0;
    
    /* Write to circular buffer */
    for (uint32_t i = 0; i < len; i++) {
        uint32_t next_index = (rtt_up_write_index + 1) % RTT_UP_BUF_SIZE;
        
        /* Check if buffer is full */
        if (next_index == rtt_up_read_index)
            break;
        
        rtt_up_buffer[rtt_up_write_index] = buf[i];
        rtt_up_write_index = next_index;
        written++;
    }
    
    if (written > 0)
        rtt_write_total += written;
    
    /* Note: Data will be sent to USB in aux_serial_uart_poll() main loop */
    
    return written;
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

/* Get number of bytes available in RTT up buffer */
uint32_t rtt_get_available(void)
{
    if (rtt_up_write_index >= rtt_up_read_index)
        return rtt_up_write_index - rtt_up_read_index;
    else
        return RTT_UP_BUF_SIZE - rtt_up_read_index + rtt_up_write_index;
}

/* Read from RTT up buffer to send via USB */
uint32_t rtt_read_buffer(char *buf, uint32_t max_len)
{
    if (!buf || max_len == 0)
        return 0;
    
    uint32_t read = 0;
    
    while (read < max_len && rtt_up_read_index != rtt_up_write_index) {
        buf[read++] = rtt_up_buffer[rtt_up_read_index];
        rtt_up_read_index = (rtt_up_read_index + 1) % RTT_UP_BUF_SIZE;
    }
    
    rtt_read_total += read;
    
    return read;
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

/* Debug functions to track data flow */
uint32_t rtt_get_write_total(void)
{
    return rtt_write_total;
}

uint32_t rtt_get_read_total(void)
{
    return rtt_read_total;
}
