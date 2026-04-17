/*
 * Copyright (c) 2025 BlackMagic Port for HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AUX_SERIAL_H
#define AUX_SERIAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declaration - actual definition in usbd_cdc_acm.h */
struct cdc_line_coding;

/* Initialize aux serial (UART pass-through) */
void aux_serial_init(void);

/* Enable UART pins (PA08/PA09) - call only in SWD mode to avoid JTAG conflict */
void aux_serial_enable_pins(void);

/* Check if UART pins are enabled */
bool aux_serial_pins_enabled(void);

/* Set/get line coding for UART */
void aux_serial_set_encoding(const struct cdc_line_coding *coding);
void aux_serial_get_encoding(struct cdc_line_coding *coding);

/* Get current transmit buffer */
char *aux_serial_current_transmit_buffer(void);

/* Get transmit buffer fullness */
size_t aux_serial_transmit_buffer_fullness(void);

/* Send data from transmit buffer */
void aux_serial_send(size_t len);

/* Check if receive buffer is empty */
bool aux_serial_receive_buffer_empty(void);

/* Update receive buffer fullness */
void aux_serial_update_receive_buffer_fullness(void);

/* Drain (clear) receive buffer */
void aux_serial_drain_receive_buffer(void);

/* Stage receive buffer */
void aux_serial_stage_receive_buffer(void);

/* Get character from aux serial (non-blocking, returns -1 if no data) */
int32_t aux_serial_getchar(void);

/* Helper functions (called from cdc_acm_dual.c) */
void aux_serial_usb_send(size_t len);
void aux_serial_restart_rx(void);
void aux_serial_uart_poll(void);

#endif /* AUX_SERIAL_H */
