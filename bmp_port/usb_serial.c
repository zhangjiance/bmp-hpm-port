/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011 Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2022-2024 1BitSquared <info@1bitsquared.com>
 * Written by Rachel Mant <git@dragonmux.network>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This file implements a the USB Communications Device Class - Abstract
 * Control Model (CDC-ACM) as defined in CDC PSTN subclass 1.2.
 * A Device Firmware Upgrade (DFU 1.1) class interface is provided for
 * field firmware upgrade.
 *
 * The device's unique id is used as the USB serial number string.
 *
 * Endpoint Usage
 *
 *     0 Control Endpoint
 * IN  1 GDB CDC DATA
 * OUT 1 GDB CDC DATA
 * IN  2 GDB CDC CTR
 * IN  3 UART CDC DATA
 * OUT 3 UART CDC DATA
 * OUT 4 UART CDC CTRL
 * In  5 Trace Capture
 *
 */

#include "general.h"
#include "platform.h"
#include "gdb_if.h"
#include <stdbool.h>
#include "usb_serial.h"
#ifdef PLATFORM_HAS_TRACESWO
#include "swo.h"
#endif
#include "rtt.h"
#include "rtt_if.h"



bool gdb_serial_get_dtr(void)
{
	return true;
}


// void usb_serial_set_state(usbd_device *const dev, const uint16_t iface, const uint8_t ep)
// {
// }

// void usb_serial_set_config(usbd_device *dev, uint16_t value)
// {
// }

void debug_serial_send_stdout(const uint8_t *const data, const size_t len)
{
}

uint32_t debug_serial_fifo_send(const char *const fifo, const uint32_t fifo_begin, const uint32_t fifo_end)
{
	return fifo_begin;
}

