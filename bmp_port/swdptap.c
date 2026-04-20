/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
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

/* This file implements the SW-DP interface. */

#include "general.h"
#include "platform.h"
#include "timing.h"
#include "swd.h"
#include "maths_utils.h"
#include "jtag_port.h"
#include "hpm_spi_accel.h"
#include "hpm_spi_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_iomux.h"

swd_proc_s swd_proc;

/* =================================================================
 * SPI-accelerated SWD (SPI1: PA27=SCLK, PA28=MISO, PA29=MOSI)
 * PB11 is physically connected to PA27 on PCB (same SWDCLK net)
 * ================================================================= */

static void swd_spi_pins_setup(void)
{
clock_add_to_group(SWD_SPI_BASE_CLOCK_NAME, 0);
HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL =
IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;
HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;
HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL =
IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3) |
IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);
HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
/* PB11 (same SWDCLK net) also drives SPI2 SCLK */
HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL =
IOC_PB11_FUNC_CTL_SPI2_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
}

static void swd_gpio_pins_setup(void)
{
/* Restore PA27/28/29 and PB11 back to GPIO functions */
HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_GPIO_A_27;
HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_GPIO_A_28;
HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;
HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL =
IOC_PAD_PAD_CTL_SR_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3) |
IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);
HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
HPM_IOC->PAD[IOC_PAD_PB11].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
/* Restore GPIO directions */
gpio_set_pin_output(PIN_GPIO, TCK_PORT_IDX, TCK_PIN_IDX);
gpio_set_pin_output(PIN_GPIO, TMS_PORT_IDX, TMS_PIN_IDX);
}

static void swd_spi_init_with_freq(uint32_t freq_hz)
{
spi_timing_config_t  timing_config  = {0};
spi_format_config_t  format_config  = {0};
spi_control_config_t control_config = {0};

clock_set_source_divider(SWD_SPI_BASE_CLOCK_NAME, clk_src_pll1_clk0, 10U); /* 80 MHz src */
uint32_t spi_clock = clock_get_frequency(SWD_SPI_BASE_CLOCK_NAME);

spi_master_get_default_timing_config(&timing_config);
timing_config.master_config.cs2sclk            = spi_cs2sclk_half_sclk_1;
timing_config.master_config.csht               = spi_csht_half_sclk_1;
timing_config.master_config.clk_src_freq_in_hz = spi_clock;
timing_config.master_config.sclk_freq_in_hz    = freq_hz;
if (status_success != spi_master_timing_init(SWD_SPI_BASE, &timing_config))
spi_master_set_sclk_div(SWD_SPI_BASE, 0xFF);

spi_master_get_default_format_config(&format_config);
format_config.master_config.addr_len_in_bytes = 1U;
format_config.common_config.data_len_in_bits  = 1;
format_config.common_config.data_merge        = false;
format_config.common_config.mosi_bidir        = true;  /* PA29 bidirectional SWDIO */
format_config.common_config.lsb               = true;
format_config.common_config.mode              = spi_master_mode;
format_config.common_config.cpol              = spi_sclk_low_idle;
format_config.common_config.cpha              = spi_sclk_sampling_odd_clk_edges;
spi_format_init(SWD_SPI_BASE, &format_config);

spi_master_get_default_control_config(&control_config);
control_config.master_config.cmd_enable        = false;
control_config.master_config.addr_enable       = false;
control_config.common_config.trans_mode        = spi_trans_write_dummy_read;
control_config.common_config.data_phase_fmt    = spi_single_io_mode;
control_config.common_config.dummy_cnt         = spi_dummy_count_1;
spi_control_init(SWD_SPI_BASE, &control_config, 1, 1);

/* Default: SWDIO driven (output, DIR=1) */
PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].SET = SWDIO_DIR_PIN_MASK;
}

static inline void swd_spi_reset(void)
{
SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
while (SWD_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
;
}

static void swd_spi_write_bits(uint32_t data, uint16_t nbits)
{
if (!nbits)
return;
swd_spi_reset();
PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].SET = SWDIO_DIR_PIN_MASK;
SWD_SPI_BASE->TRANSCTRL = (SWD_SPI_BASE->TRANSCTRL &
~(SPI_TRANSCTRL_TRANSMODE_MASK | SPI_TRANSCTRL_WRTRANCNT_MASK)) |
SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_only) |
SPI_TRANSCTRL_WRTRANCNT_SET(1 - 1);
spi_set_data_bits(SWD_SPI_BASE, nbits);
SWD_SPI_BASE->CMD  = 0xFF;
SWD_SPI_BASE->DATA = data;
while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
;
}

static uint32_t swd_spi_read_bits(uint16_t nbits)
{
if (!nbits)
return 0;
swd_spi_reset();
PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].CLEAR = SWDIO_DIR_PIN_MASK;
SWD_SPI_BASE->TRANSCTRL = (SWD_SPI_BASE->TRANSCTRL &
~(SPI_TRANSCTRL_TRANSMODE_MASK | SPI_TRANSCTRL_RDTRANCNT_MASK)) |
SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_read_only) |
SPI_TRANSCTRL_RDTRANCNT_SET(1 - 1);
spi_set_data_bits(SWD_SPI_BASE, nbits);
SWD_SPI_BASE->CMD = 0xFF;
while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
;
uint32_t result = SWD_SPI_BASE->DATA;
while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
;
return result;
}

/* =================================================================
 * SPI-mode SWD primitives
 * ================================================================= */

static uint32_t swdptap_seq_in_spi(size_t clock_cycles);
static bool swdptap_seq_in_parity_spi(uint32_t *ret, size_t clock_cycles);
static void swdptap_seq_out_spi(uint32_t tms_states, size_t clock_cycles);
static void swdptap_seq_out_parity_spi(uint32_t tms_states, size_t clock_cycles);

static uint32_t swdptap_seq_in_spi(size_t clock_cycles)
{
if (!clock_cycles)
return 0;
return swd_spi_read_bits((uint16_t)clock_cycles);
}

static bool swdptap_seq_in_parity_spi(uint32_t *ret, size_t clock_cycles)
{
uint32_t result    = swdptap_seq_in_spi(clock_cycles);
uint32_t parity_bit = swd_spi_read_bits(1) & 1U;
PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].SET = SWDIO_DIR_PIN_MASK;
*ret = result;
return calculate_odd_parity(result) == (bool)parity_bit;
}

static void swdptap_seq_out_spi(uint32_t tms_states, size_t clock_cycles)
{
if (!clock_cycles)
return;
swd_spi_write_bits(tms_states, (uint16_t)clock_cycles);
}

static void swdptap_seq_out_parity_spi(uint32_t tms_states, size_t clock_cycles)
{
const bool parity = calculate_odd_parity(tms_states);
swdptap_seq_out_spi(tms_states, clock_cycles);
swd_spi_write_bits(parity ? 1U : 0U, 1);
}

/* =================================================================
 * GPIO bitbang SWD
 * ================================================================= */

typedef enum swdio_status_e {
SWDIO_STATUS_FLOAT = 0,
SWDIO_STATUS_DRIVE
} swdio_status_t;

static void swdptap_turnaround(swdio_status_t dir) __attribute__((optimize(3)));
static uint32_t swdptap_seq_in_gpio(size_t clock_cycles) __attribute__((optimize(3)));
static bool swdptap_seq_in_parity_gpio(uint32_t *ret, size_t clock_cycles) __attribute__((optimize(3)));
static void swdptap_seq_out_gpio(uint32_t tms_states, size_t clock_cycles) __attribute__((optimize(3)));
static void swdptap_seq_out_parity_gpio(uint32_t tms_states, size_t clock_cycles) __attribute__((optimize(3)));

static void swdptap_turnaround(const swdio_status_t dir)
{
static swdio_status_t olddir = SWDIO_STATUS_FLOAT;
if (dir == olddir)
return;
olddir = dir;

if (dir == SWDIO_STATUS_FLOAT) {
PIN_TMS_SWDIO_SET_IN();
}
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_SET();
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_CLR();
if (dir == SWDIO_STATUS_DRIVE) {
PIN_TMS_SWDIO_SET_OUT();
}
}

static uint32_t swdptap_seq_in_clk_delay(size_t clock_cycles) __attribute__((optimize(3)));
static uint32_t swdptap_seq_in_no_delay(size_t clock_cycles) __attribute__((optimize(3)));

static uint32_t swdptap_seq_in_clk_delay(const size_t clock_cycles)
{
uint32_t value = 0;
if (!clock_cycles)
return 0;
for (size_t cycle = clock_cycles; cycle--;) {
for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
continue;
const bool bit = !!PIN_TMS_SWDIO_IN();
PIN_SWCLK_TCK_SET();
for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
continue;
value >>= 1U;
value |= (uint32_t)bit << 31U;
PIN_SWCLK_TCK_CLR();
}
value >>= (32U - clock_cycles);
return value;
}

static uint32_t swdptap_seq_in_no_delay(const size_t clock_cycles)
{
uint32_t value = 0;
if (!clock_cycles)
return 0;
for (size_t cycle = clock_cycles; cycle--;) {
uint32_t bit = PIN_TMS_SWDIO_IN();
PIN_SWCLK_TCK_SET();
value >>= 1U;
value |= bit << 31U;
PIN_SWCLK_TCK_CLR();
}
value >>= (32U - clock_cycles);
return value;
}

static uint32_t swdptap_seq_in_gpio(size_t clock_cycles)
{
swdptap_turnaround(SWDIO_STATUS_FLOAT);
if (target_clk_divider != UINT32_MAX)
return swdptap_seq_in_clk_delay(clock_cycles);
else // NOLINT(readability-else-after-return)
return swdptap_seq_in_no_delay(clock_cycles);
}

static bool swdptap_seq_in_parity_gpio(uint32_t *ret, size_t clock_cycles)
{
const uint32_t result = swdptap_seq_in_gpio(clock_cycles);
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
const uint32_t bit = PIN_TMS_SWDIO_IN();
PIN_SWCLK_TCK_SET();
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_CLR();
swdptap_turnaround(SWDIO_STATUS_DRIVE);
*ret = result;
const bool parity = calculate_odd_parity(result);
return parity == (bool)bit;
}

static void swdptap_seq_out_clk_delay(uint32_t tms_states, size_t clock_cycles) __attribute__((optimize(3)));
static void swdptap_seq_out_no_delay(uint32_t tms_states, size_t clock_cycles) __attribute__((optimize(3)));

static void swdptap_seq_out_clk_delay(const uint32_t tms_states, const size_t clock_cycles)
{
uint32_t value = tms_states;
for (size_t cycle = clock_cycles; cycle--;) {
PIN_TMS_SWDIO_OUT(value & 1U);
for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_SET();
for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
continue;
value >>= 1U;
PIN_SWCLK_TCK_CLR();
}
}

static void swdptap_seq_out_no_delay(const uint32_t tms_states, const size_t clock_cycles)
{
uint32_t value = tms_states;
if (!clock_cycles)
return;
for (size_t cycle = clock_cycles; cycle--;) {
PIN_SWCLK_TCK_CLR();
PIN_TMS_SWDIO_OUT(value & 1U);
PIN_SWCLK_TCK_SET();
value >>= 1U;
}
PIN_SWCLK_TCK_CLR();
}

static void swdptap_seq_out_gpio(const uint32_t tms_states, const size_t clock_cycles)
{
swdptap_turnaround(SWDIO_STATUS_DRIVE);
if (target_clk_divider != UINT32_MAX)
swdptap_seq_out_clk_delay(tms_states, clock_cycles);
else
swdptap_seq_out_no_delay(tms_states, clock_cycles);
}

static void swdptap_seq_out_parity_gpio(const uint32_t tms_states, const size_t clock_cycles)
{
const bool parity = calculate_odd_parity(tms_states);
swdptap_seq_out_gpio(tms_states, clock_cycles);
PIN_TMS_SWDIO_OUT(parity);
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_SET();
for (volatile uint32_t counter = target_clk_divider + 1; counter > 0; --counter)
continue;
PIN_SWCLK_TCK_CLR();
}

/* =================================================================
 * Public: hardware setup + function-pointer assignment
 * Called from swdptap_init() and from monitor mode-switch command.
 * Does NOT send any SWD sequences.
 * ================================================================= */

void hpm_swd_setup_mode(void)
{
if (hpm_use_spi_mode) {
swd_spi_pins_setup();
swd_spi_init_with_freq(hpm_spi_freq_hz);
swd_proc.seq_in         = swdptap_seq_in_spi;
swd_proc.seq_in_parity  = swdptap_seq_in_parity_spi;
swd_proc.seq_out        = swdptap_seq_out_spi;
swd_proc.seq_out_parity = swdptap_seq_out_parity_spi;
} else {
swd_gpio_pins_setup();
swd_proc.seq_in         = swdptap_seq_in_gpio;
swd_proc.seq_in_parity  = swdptap_seq_in_parity_gpio;
swd_proc.seq_out        = swdptap_seq_out_gpio;
swd_proc.seq_out_parity = swdptap_seq_out_parity_gpio;
}
}

/* =================================================================
 * swdptap_init: hardware setup (no SWD sequences needed for SWD)
 * ================================================================= */

void swdptap_init(void)
{
extern void aux_serial_enable_pins(void);
extern void uninit_jtag_tdi_pin(void);

aux_serial_enable_pins();
uninit_jtag_tdi_pin();

hpm_swd_setup_mode();
}
