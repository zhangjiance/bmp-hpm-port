/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2022-2023 1BitSquared <info@1bitsquared.com>
 * Modified by Rachel Mant <git@dragonmux.network>
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

/* This file implements the low-level JTAG TAP interface.  */

#include <stdio.h>

#include "adiv5.h"
#include "board.h"
#include "general.h"
#include "hpm_clock_drv.h"
#include "hpm_iomux.h"
#include "hpm_spi_accel.h"
#include "hpm_spi_drv.h"
#include "jtag_port.h"
#include "jtagtap.h"
#include "platform.h"

jtag_proc_s jtag_proc;

/* =================================================================
 * SPI-accelerated JTAG (SPI2: PB11=SCLK, PB12=MISO, PB13=MOSI)
 * ================================================================= */

static void jtag_spi_pins_setup(void) {
  clock_add_to_group(JTAG_SPI_BASE_CLOCK_NAME, 0);
  HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL =
      IOC_PB11_FUNC_CTL_SPI2_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
  HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_SPI2_MISO;
  HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_SPI2_MOSI;
  HPM_IOC->PAD[IOC_PAD_PB11].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
  HPM_IOC->PAD[IOC_PAD_PB12].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
  HPM_IOC->PAD[IOC_PAD_PB13].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
}

static void jtag_gpio_pins_setup(void) {
  /* Restore PB11/12/13 to GPIO functions */
  HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
  HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_GPIO_B_12;
  HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_GPIO_B_13;
  HPM_IOC->PAD[IOC_PAD_PB11].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
  HPM_IOC->PAD[IOC_PAD_PB12].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
  HPM_IOC->PAD[IOC_PAD_PB13].PAD_CTL =
      IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
  /* Restore GPIO directions */
  gpio_set_pin_output(PIN_GPIO, TCK_PORT_IDX, TCK_PIN_IDX);
  gpio_set_pin_input(PIN_GPIO, TDO_PORT_IDX, TDO_PIN_IDX);
  gpio_set_pin_output(PIN_GPIO, TDI_PORT_IDX, TDI_PIN_IDX);
}

static void jtag_spi_init_with_freq(uint32_t freq_hz) {
  spi_timing_config_t timing_config = {0};
  spi_format_config_t format_config = {0};
  spi_control_config_t control_config = {0};

  uint32_t spi_clock = clock_get_frequency(JTAG_SPI_BASE_CLOCK_NAME);

  spi_master_get_default_timing_config(&timing_config);
  timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_1;
  timing_config.master_config.csht = spi_csht_half_sclk_1;
  timing_config.master_config.clk_src_freq_in_hz = spi_clock;
  timing_config.master_config.sclk_freq_in_hz = freq_hz;
  if (status_success != spi_master_timing_init(JTAG_SPI_BASE, &timing_config))
    spi_master_set_sclk_div(JTAG_SPI_BASE, 0xFF);

  spi_master_get_default_format_config(&format_config);
  format_config.master_config.addr_len_in_bytes = 1U;
  format_config.common_config.data_len_in_bits = 8;
  format_config.common_config.data_merge = false;
  format_config.common_config.mosi_bidir = false;
  format_config.common_config.lsb = true;
  format_config.common_config.mode = spi_master_mode;
  format_config.common_config.cpol = spi_sclk_low_idle;
  format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;
  spi_format_init(JTAG_SPI_BASE, &format_config);

  spi_master_get_default_control_config(&control_config);
  control_config.master_config.cmd_enable = false;
  control_config.master_config.addr_enable = false;
  control_config.common_config.trans_mode = spi_trans_write_read_together;
  control_config.common_config.data_phase_fmt = spi_single_io_mode;
  control_config.common_config.dummy_cnt = spi_dummy_count_1;
  spi_control_init(JTAG_SPI_BASE, &control_config, 1, 1);
}

/* Send up to 32 bits via SPI (write-only) to generate TCK clocks */
static void jtag_spi_clk_bits(uint16_t nbits, uint32_t data) {
  if (!nbits)
    return;
  JTAG_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
  while (JTAG_SPI_BASE->STATUS &
         (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    ;
  spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
  spi_set_data_bits(JTAG_SPI_BASE, nbits);
  spi_set_write_data_count(JTAG_SPI_BASE, 1);
  JTAG_SPI_BASE->CMD = 0xFF;
  JTAG_SPI_BASE->DATA = data;
  while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    ;
}

/* Bulk TDI/TDO transfer via SPI, optionally capturing TDO */
static void jtag_spi_sequence_inner(uint32_t nbits, const uint8_t *tdi,
                                    uint8_t *tdo) {
  uint32_t nb_bytes = nbits / 8;
  uint32_t remain_bits = nbits % 8;
  uint32_t rx_idx = 0, tx_idx = 0;

  JTAG_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
  while (JTAG_SPI_BASE->STATUS &
         (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    ;

  if (nb_bytes) {
    spi_set_data_bits(JTAG_SPI_BASE, 8);
    spi_set_write_data_count(JTAG_SPI_BASE, nb_bytes);
    spi_set_read_data_count(JTAG_SPI_BASE, nb_bytes);
    if (tdo) {
      spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_read_together);
    } else {
      spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
      rx_idx = nb_bytes;
    }
    JTAG_SPI_BASE->CMD = 0xFF;
    while ((rx_idx < nb_bytes) || (tx_idx < nb_bytes)) {
      if (tx_idx < nb_bytes &&
          !(JTAG_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK))
        JTAG_SPI_BASE->DATA = tdi[tx_idx++];
      if (tdo && rx_idx < nb_bytes &&
          !(JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK))
        tdo[rx_idx++] = (uint8_t)JTAG_SPI_BASE->DATA;
    }
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
      ;
  }

  if (remain_bits) {
    spi_set_data_bits(JTAG_SPI_BASE, remain_bits);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_transfer_mode(JTAG_SPI_BASE, tdo ? spi_trans_write_read_together
                                             : spi_trans_write_only);
    JTAG_SPI_BASE->CMD = 0xFF;
    JTAG_SPI_BASE->DATA = tdi[nb_bytes];
    if (tdo) {
      while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) ==
             SPI_STATUS_RXEMPTY_MASK)
        ;
      tdo[nb_bytes] = (uint8_t)JTAG_SPI_BASE->DATA;
    }
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
      ;
  }
}

/* SPI-mode bulk primitives */
static void jtagtap_tms_seq_spi(uint32_t tms_states, size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_tdo_seq_spi(const uint8_t *data_in, uint8_t *data_out,
                                    bool final_tms, size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_seq_spi(const uint8_t *data_in, bool final_tms,
                                size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_cycle_spi(size_t clock_cycles) __attribute__((optimize(3)));

static void jtagtap_tms_seq_spi(uint32_t tms_states,
                                const size_t clock_cycles) {
  size_t cycle = 0;
  while (cycle < clock_cycles) {
    bool tms = (tms_states >> cycle) & 1U;
    size_t run = 1;
    while (cycle + run < clock_cycles &&
           (bool)((tms_states >> (cycle + run)) & 1U) == tms && run < 32U)
      run++;
    PIN_TMS_SWDIO_OUT(tms);
    jtag_spi_clk_bits((uint16_t)run, 0xFFFFFFFFU);
    cycle += run;
  }
}

static void jtagtap_tdi_tdo_seq_spi(const uint8_t *const data_in,
                                    uint8_t *const data_out,
                                    const bool final_tms,
                                    const size_t clock_cycles) {
  if (!clock_cycles)
    return;
  size_t pre_cycles = clock_cycles - 1U;
  if (pre_cycles) {
    PIN_TMS_SWDIO_OUT(false);
    jtag_spi_sequence_inner(pre_cycles, data_in, data_out);
  }
  /* Last bit with final_tms */
  PIN_TMS_SWDIO_OUT(final_tms);
  const size_t last_byte = (clock_cycles - 1U) >> 3U;
  const uint8_t last_bit = (clock_cycles - 1U) & 7U;
  uint8_t tdi_last = (data_in[last_byte] >> last_bit) & 1U;
  uint8_t tdo_last = 0;
  jtag_spi_sequence_inner(1, &tdi_last, data_out ? &tdo_last : NULL);
  if (data_out) {
    data_out[last_byte] &= ~(1U << last_bit);
    data_out[last_byte] |= (uint8_t)(tdo_last << last_bit);
  }
}

static void jtagtap_tdi_seq_spi(const uint8_t *const data_in,
                                const bool final_tms,
                                const size_t clock_cycles) {
  jtagtap_tdi_tdo_seq_spi(data_in, NULL, final_tms, clock_cycles);
}

static void jtagtap_cycle_spi(const size_t clock_cycles) {
  size_t done = 0;
  while (done < clock_cycles) {
    size_t chunk = clock_cycles - done;
    if (chunk > 32U)
      chunk = 32U;
    jtag_spi_clk_bits((uint16_t)chunk, 0xFFFFFFFFU);
    done += chunk;
  }
}

/* =================================================================
 * GPIO bitbang JTAG
 * ================================================================= */

static void jtagtap_tms_seq_clk_delay(uint32_t tms_states, size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tms_seq_no_delay(uint32_t tms_states, size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_tdo_seq_clk_delay(const uint8_t *data_in,
                                          uint8_t *data_out, bool final_tms,
                                          size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_tdo_seq_no_delay(const uint8_t *data_in,
                                         uint8_t *data_out, bool final_tms,
                                         size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_seq_clk_delay(const uint8_t *data_in, bool final_tms,
                                      size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_tdi_seq_no_delay(const uint8_t *data_in, bool final_tms,
                                     size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_cycle_clk_delay(size_t clock_cycles)
    __attribute__((optimize(3)));
static void jtagtap_cycle_no_delay(size_t clock_cycles)
    __attribute__((optimize(3)));

static void jtagtap_tms_seq_clk_delay(uint32_t tms_states,
                                      const size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    const bool state = tms_states & 1U;
    PIN_TMS_SWDIO_OUT(state);
    PIN_SWCLK_TCK_SET();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
    tms_states >>= 1U;
    PIN_SWCLK_TCK_CLR();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
  }
}

static void jtagtap_tms_seq_no_delay(uint32_t tms_states,
                                     const size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    PIN_TMS_SWDIO_OUT(tms_states & 1U);
    PIN_SWCLK_TCK_SET();
    tms_states >>= 1U;
    PIN_SWCLK_TCK_CLR();
  }
}

static void jtagtap_tdi_tdo_seq_clk_delay(const uint8_t *const data_in,
                                          uint8_t *const data_out,
                                          const bool final_tms,
                                          const size_t clock_cycles) {
  uint8_t value = 0;
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    const uint8_t bit = cycle & 7U;
    const size_t byte = cycle >> 3U;
    PIN_TMS_SWDIO_OUT(cycle + 1U >= clock_cycles && final_tms);
    PIN_TDI_OUT(data_in[byte] & (1U << bit));
    PIN_SWCLK_TCK_SET();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
    if (PIN_TDO_IN())
      value |= 1U << bit;
    if (bit == 7U) {
      data_out[byte] = value;
      value = 0;
    }
    PIN_SWCLK_TCK_CLR();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
  }
  if (clock_cycles & 7U)
    data_out[(clock_cycles - 1U) >> 3U] = value;
}

static void jtagtap_tdi_tdo_seq_no_delay(const uint8_t *const data_in,
                                         uint8_t *const data_out,
                                         const bool final_tms,
                                         const size_t clock_cycles) {
  uint8_t value = 0;
  for (size_t cycle = 0; cycle < clock_cycles;) {
    const uint8_t bit = cycle & 7U;
    const size_t byte = cycle >> 3U;
    const bool tms = cycle + 1U >= clock_cycles && final_tms;
    const bool tdi = !!(data_in[byte] & (1U << bit));
    PIN_SWCLK_TCK_CLR();
    PIN_TDI_OUT(tdi);
    PIN_TMS_SWDIO_OUT(tms);
    ++cycle;
    PIN_SWCLK_TCK_SET();
    value |= (uint8_t)(PIN_TDO_IN() << bit);
    if (bit == 7U) {
      data_out[byte] = value;
      value = 0;
    }
  }
  if (clock_cycles & 7U)
    data_out[(clock_cycles - 1U) >> 3U] = value;
  PIN_SWCLK_TCK_CLR();
}

static void jtagtap_tdi_seq_clk_delay(const uint8_t *const data_in,
                                      const bool final_tms,
                                      size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    const uint8_t bit = cycle & 7U;
    const size_t byte = cycle >> 3U;
    PIN_TMS_SWDIO_OUT(cycle + 1U >= clock_cycles && final_tms);
    PIN_TDI_OUT(data_in[byte] & (1U << bit));
    PIN_SWCLK_TCK_SET();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
    PIN_SWCLK_TCK_CLR();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
  }
}

static void jtagtap_tdi_seq_no_delay(const uint8_t *const data_in,
                                     const bool final_tms,
                                     size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles;) {
    const uint8_t bit = cycle & 7U;
    const size_t byte = cycle >> 3U;
    const bool tms = cycle + 1U >= clock_cycles && final_tms;
    const bool tdi = !!(data_in[byte] & (1U << bit));
    PIN_SWCLK_TCK_CLR();
    PIN_TMS_SWDIO_OUT(tms);
    PIN_TDI_OUT(tdi);
    ++cycle;
    PIN_SWCLK_TCK_SET();
  }
  PIN_SWCLK_TCK_CLR();
}

static void jtagtap_cycle_clk_delay(const size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    PIN_SWCLK_TCK_SET();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
    PIN_SWCLK_TCK_CLR();
    for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
      continue;
  }
}

static void jtagtap_cycle_no_delay(const size_t clock_cycles) {
  for (size_t cycle = 0; cycle < clock_cycles; ++cycle) {
    PIN_SWCLK_TCK_SET();
    PIN_SWCLK_TCK_CLR();
  }
}

/* =================================================================
 * Mode-dispatching top-level JTAG primitives
 * ================================================================= */

static void jtagtap_reset(void);
static void jtagtap_tms_seq(uint32_t tms_states, size_t clock_cycles);
static void jtagtap_tdi_tdo_seq(uint8_t *data_out, bool final_tms,
                                const uint8_t *data_in, size_t clock_cycles);
static void jtagtap_tdi_seq(bool final_tms, const uint8_t *data_in,
                            size_t clock_cycles);
static bool jtagtap_next(bool tms, bool tdi);
static void jtagtap_cycle(bool tms, bool tdi, size_t clock_cycles);

static bool jtagtap_next_clk_delay(void) __attribute__((optimize(3)));
static bool jtagtap_next_no_delay(void) __attribute__((optimize(3)));

static bool jtagtap_next_clk_delay(void) {
  PIN_SWCLK_TCK_SET();
  for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
    continue;
  const uint16_t result = (uint16_t)PIN_TDO_IN();
  PIN_SWCLK_TCK_CLR();
  for (volatile uint32_t counter = target_clk_divider; counter > 0; --counter)
    continue;
  return result != 0;
}

static bool jtagtap_next_no_delay(void) {
  PIN_SWCLK_TCK_SET();
  const uint16_t result = (uint16_t)PIN_TDO_IN();
  PIN_SWCLK_TCK_CLR();
  return result != 0;
}

static bool jtagtap_next(const bool tms, const bool tdi) {
  if (hpm_use_spi_mode) {
    /* Temporarily switch TCK/TDI/TDO back to GPIO for single-bit transfer */
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
    HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_GPIO_B_12;
    HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_GPIO_B_13;
  }

  PIN_TMS_SWDIO_OUT(tms);
  PIN_TDI_OUT(tdi);
  bool result;
  if (target_clk_divider != UINT32_MAX)
    result = jtagtap_next_clk_delay();
  else
    result = jtagtap_next_no_delay();

  if (hpm_use_spi_mode) {
    /* Restore SPI pin mux */
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL =
        IOC_PB11_FUNC_CTL_SPI2_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
    HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_SPI2_MISO;
    HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_SPI2_MOSI;
  }
  return result;
}

static void jtagtap_tms_seq(const uint32_t tms_states,
                            const size_t clock_cycles) {
  PIN_TDI_SET();
  if (hpm_use_spi_mode) {
    jtagtap_tms_seq_spi(tms_states, clock_cycles);
  } else {
    if (target_clk_divider != UINT32_MAX)
      jtagtap_tms_seq_clk_delay(tms_states, clock_cycles);
    else
      jtagtap_tms_seq_no_delay(tms_states, clock_cycles);
  }
}

static void jtagtap_tdi_tdo_seq(uint8_t *const data_out, const bool final_tms,
                                const uint8_t *const data_in,
                                size_t clock_cycles) {
  PIN_TMS_SWDIO_CLR();
  PIN_TDI_CLR();
  if (hpm_use_spi_mode) {
    jtagtap_tdi_tdo_seq_spi(data_in, data_out, final_tms, clock_cycles);
  } else {
    if (target_clk_divider != UINT32_MAX)
      jtagtap_tdi_tdo_seq_clk_delay(data_in, data_out, final_tms, clock_cycles);
    else
      jtagtap_tdi_tdo_seq_no_delay(data_in, data_out, final_tms, clock_cycles);
  }
}

static void jtagtap_tdi_seq(const bool final_tms, const uint8_t *const data_in,
                            const size_t clock_cycles) {
  PIN_TMS_SWDIO_CLR();
  if (hpm_use_spi_mode) {
    jtagtap_tdi_seq_spi(data_in, final_tms, clock_cycles);
  } else {
    if (target_clk_divider != UINT32_MAX)
      jtagtap_tdi_seq_clk_delay(data_in, final_tms, clock_cycles);
    else
      jtagtap_tdi_seq_no_delay(data_in, final_tms, clock_cycles);
  }
}

static void jtagtap_cycle(const bool tms, const bool tdi,
                          const size_t clock_cycles) {
  jtagtap_next(tms, tdi);
  if (hpm_use_spi_mode) {
    jtagtap_cycle_spi(clock_cycles - 1U);
  } else {
    if (target_clk_divider != UINT32_MAX)
      jtagtap_cycle_clk_delay(clock_cycles - 1U);
    else
      jtagtap_cycle_no_delay(clock_cycles - 1U);
  }
}

/* =================================================================
 * Public: hardware setup + function-pointer assignment
 * Called from jtagtap_init() and from monitor mode-switch command.
 * Does NOT send any JTAG sequences.
 * ================================================================= */

void hpm_jtag_setup_mode(void) {
  /* TMS/TRST/SRST always GPIO; SWDIO_DIR=1 in JTAG */
  PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].SET = SWDIO_DIR_PIN_MASK;

  if (hpm_use_spi_mode) {
    jtag_spi_pins_setup();
    jtag_spi_init_with_freq(hpm_spi_freq_hz);
  } else {
    jtag_gpio_pins_setup();
  }

  jtag_proc.jtagtap_reset = jtagtap_reset;
  jtag_proc.jtagtap_next = jtagtap_next;
  jtag_proc.jtagtap_tms_seq = jtagtap_tms_seq;
  jtag_proc.jtagtap_tdi_tdo_seq = jtagtap_tdi_tdo_seq;
  jtag_proc.jtagtap_tdi_seq = jtagtap_tdi_seq;
  jtag_proc.jtagtap_cycle = jtagtap_cycle;
  jtag_proc.tap_idle_cycles = 1;
}

/* =================================================================
 * jtagtap_init: hardware setup + JTAG reset/selection sequences
 * ================================================================= */

static void jtagtap_reset(void) {
#ifdef PIN_JTAG_TRST
  PIN_nTRST_CLR();
  for (volatile size_t i = 0; i < 10000U; i++)
    continue;
  PIN_nTRST_SET();
#endif
  jtagtap_soft_reset();
}

void jtagtap_init(void) {
  platform_target_clk_output_enable(true);

  extern void aux_serial_disable_pins(void);
  aux_serial_disable_pins();

  hpm_jtag_setup_mode();

  /* JTAG reset and dormant-to-JTAG selection sequences */
  jtagtap_cycle(true, false, 51U);
  jtagtap_tms_seq(ADIV5_SWD_TO_JTAG_SELECT_SEQUENCE, 16U);
  jtagtap_cycle(true, false, 51U);
  jtagtap_tms_seq(ADIV5_SWD_TO_DORMANT_SEQUENCE, 16U);
  jtagtap_tms_seq(0xffU, 8U);
  jtagtap_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_0, 32U);
  jtagtap_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_1, 32U);
  jtagtap_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_2, 32U);
  jtagtap_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_3, 32U);
  jtagtap_tms_seq(ADIV5_ACTIVATION_CODE_ARM_JTAG_DP << 4U, 12U);
}
