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
#include "hpm_clock_drv.h"
#include "hpm_iomux.h"
#include "hpm_spi_accel.h"
#include "hpm_spi_drv.h"
#include "jtag_port.h"
#include "maths_utils.h"
#include "platform.h"
#include "swd.h"
#include "timing.h"

swd_proc_s swd_proc;

/*
 * SPI mode SWD transfer state tracking.
 * SWD protocol requires turnaround cycles when switching SWDIO direction.
 */
typedef enum swdio_status_spi_e {
  SWDIO_SPI_STATUS_FLOAT = 0, /* SWDIO is input (target drives) */
  SWDIO_SPI_STATUS_DRIVE      /* SWDIO is output (host drives) */
} swdio_status_spi_t;

static swdio_status_spi_t swd_spi_current_dir = SWDIO_SPI_STATUS_DRIVE;

/* Track current mode to avoid redundant pin reconfigurations */
typedef enum swd_mode_e {
  SWD_MODE_NONE = 0,
  SWD_MODE_SPI,
  SWD_MODE_GPIO
} swd_mode_t;

static swd_mode_t swd_current_mode = SWD_MODE_NONE;

/* =================================================================
 * SPI-accelerated SWD (SPI1: PA27=SCLK, PA28=MISO, PA29=MOSI)
 * Hardware connections on PCB:
 *   - PA27 (SPI1_SCLK) ←→ PB11 (GPIO) → SWDCLK network
 *   - PA28 (SPI1_MISO) ←→ PA29 (SPI1_MOSI/GPIO) → SWDIO network
 * ================================================================= */

static void swd_spi_pins_setup(void) {
  /* Skip if already in SPI mode to avoid glitches */
  if (swd_current_mode == SWD_MODE_SPI)
    return;

  clock_add_to_group(SWD_SPI_BASE_CLOCK_NAME, 0);
  /* Configure SPI1 pins */
  HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL =
      IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
  HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;
  HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;
  HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = PAD_CTL_FAST;
  HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL = PAD_CTL_FAST_PULLDOWN;
  HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = PAD_CTL_FAST;
  /* PB11: Set as GPIO input (high-Z) to avoid conflict with PA27 (SPI drives
   * SWDCLK) */
  HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
  HPM_IOC->PAD[IOC_PAD_PB11].PAD_CTL = PAD_CTL_FAST;
  gpio_set_pin_input(PIN_GPIO, 1, 11); /* Port B=1, Pin 11 */
  /* Reset turnaround state for SPI mode */
  swd_spi_current_dir = SWDIO_SPI_STATUS_DRIVE;
  swd_current_mode = SWD_MODE_SPI;
}

static void swd_gpio_pins_setup(void) {
  /* Skip if already in GPIO mode to avoid glitches */
  if (swd_current_mode == SWD_MODE_GPIO)
    return;

  /* Restore all pins back to GPIO functions */
  HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_GPIO_A_27;
  HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_GPIO_A_28;
  HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;
  HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
  HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = PAD_CTL_FAST;
  HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL = PAD_CTL_FAST;
  /* PA29 (SWDIO): needs pull-down for stable reads when target drives the line */
  HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = PAD_CTL_FAST_PULLDOWN;
  HPM_IOC->PAD[IOC_PAD_PB11].PAD_CTL = PAD_CTL_FAST;
  /* GPIO mode: PB11 drives SWCLK, PA29 drives SWDIO (bidirectional)
   * Set PA27 and PA28 as input (high-Z) to avoid conflicts:
   *   - PA27 high-Z: won't conflict with PB11 on SWDCLK network
   *   - PA28 high-Z: won't conflict with PA29 on SWDIO network */
  gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(IOC_PAD_PA27),
                     GPIO_GET_PIN_INDEX(IOC_PAD_PA27));
  gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(IOC_PAD_PA28),
                     GPIO_GET_PIN_INDEX(IOC_PAD_PA28));
  gpio_set_pin_output(PIN_GPIO, TCK_PORT_IDX,
                      TCK_PIN_IDX); /* PB11 output for SWCLK */
  gpio_set_pin_output(PIN_GPIO, TMS_PORT_IDX,
                      TMS_PIN_IDX); /* PA29 output for SWDIO */
  swd_current_mode = SWD_MODE_GPIO;
}

static void swd_spi_init_with_freq(uint32_t freq_hz) {
  spi_timing_config_t timing_config = {0};
  spi_format_config_t format_config = {0};
  spi_control_config_t control_config = {0};
  clk_src_t best_clk_src;
  uint32_t best_div;
  
  /* Use dynamic clock selection algorithm to find optimal configuration */
  select_optimal_clock_config(SWD_SPI_BASE_CLOCK_NAME, freq_hz, &best_clk_src, &best_div);
  
  /* Apply the selected configuration */
  clock_set_source_divider(SWD_SPI_BASE_CLOCK_NAME, best_clk_src, best_div);
  uint32_t spi_clock = clock_get_frequency(SWD_SPI_BASE_CLOCK_NAME);

  spi_master_get_default_timing_config(&timing_config);
  timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_1;
  timing_config.master_config.csht = spi_csht_half_sclk_1;
  timing_config.master_config.clk_src_freq_in_hz = spi_clock;
  timing_config.master_config.sclk_freq_in_hz = freq_hz;
  if (status_success != spi_master_timing_init(SWD_SPI_BASE, &timing_config))
    spi_master_set_sclk_div(SWD_SPI_BASE, 0xFF);

  spi_master_get_default_format_config(&format_config);
  format_config.master_config.addr_len_in_bytes = 1U;
  format_config.common_config.data_len_in_bits = 1;
  format_config.common_config.data_merge = false;
  format_config.common_config.mosi_bidir = true; /* PA29 bidirectional SWDIO */
  format_config.common_config.lsb = true;
  format_config.common_config.mode = spi_master_mode;
  format_config.common_config.cpol = spi_sclk_low_idle;
  format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;
  spi_format_init(SWD_SPI_BASE, &format_config);

  spi_master_get_default_control_config(&control_config);
  control_config.master_config.cmd_enable = false;
  control_config.master_config.addr_enable = false;
  control_config.common_config.trans_mode = spi_trans_write_dummy_read;
  control_config.common_config.data_phase_fmt = spi_single_io_mode;
  control_config.common_config.dummy_cnt = spi_dummy_count_1;
  spi_control_init(SWD_SPI_BASE, &control_config, 1, 1);

  /* Initialize SWDIO_DIR and state tracking */
  swd_spi_current_dir = SWDIO_SPI_STATUS_DRIVE; /* Start in output mode */
  PIN_SWDIO_DIR_SET(); /* DIR=1 (output) */
}

static inline void swd_spi_reset(void) {
  SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
  while (SWD_SPI_BASE->STATUS &
         (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    ;
}

static void swd_spi_write_bits(uint32_t data, uint16_t nbits) {
  if (!nbits)
    return;
  swd_spi_reset();
  /* Note: SWDIO_DIR is controlled by swd_spi_turnaround(), not here */
  SWD_SPI_BASE->TRANSCTRL =
      (SWD_SPI_BASE->TRANSCTRL &
       ~(SPI_TRANSCTRL_TRANSMODE_MASK | SPI_TRANSCTRL_WRTRANCNT_MASK)) |
      SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_only) |
      SPI_TRANSCTRL_WRTRANCNT_SET(1 - 1);
  spi_set_data_bits(SWD_SPI_BASE, nbits);
  SWD_SPI_BASE->CMD = 0xFF;
  SWD_SPI_BASE->DATA = data;
  while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    ;
}

static uint32_t swd_spi_read_bits(uint16_t nbits) {
  if (!nbits)
    return 0;
  swd_spi_reset();
  /* Note: SWDIO_DIR is controlled by swd_spi_turnaround(), not here */
  SWD_SPI_BASE->TRANSCTRL =
      (SWD_SPI_BASE->TRANSCTRL &
       ~(SPI_TRANSCTRL_TRANSMODE_MASK | SPI_TRANSCTRL_RDTRANCNT_MASK)) |
      SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_read_only) |
      SPI_TRANSCTRL_RDTRANCNT_SET(1 - 1);
  spi_set_data_bits(SWD_SPI_BASE, nbits);
  SWD_SPI_BASE->CMD = 0xFF;
  while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) ==
         SPI_STATUS_RXEMPTY_MASK)
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
static void swdptap_seq_out_parity_spi(uint32_t tms_states,
                                       size_t clock_cycles);

/*
 * SWD protocol turnaround: 1 clock cycle with SWDIO in specified direction.
 * Turnaround is required when changing SWDIO direction between host and target.
 */
static void swd_spi_turnaround(swdio_status_spi_t new_dir) {
  if (new_dir == swd_spi_current_dir)
    return;

  swd_spi_current_dir = new_dir;

  /* Set SWDIO_DIR based on new direction */
  if (new_dir == SWDIO_SPI_STATUS_FLOAT) {
    /* Target drives SWDIO: set DIR=0 (input) */
    PIN_SWDIO_DIR_CLR();
  } else {
    /* Host drives SWDIO: set DIR=1 (output) */
    PIN_SWDIO_DIR_SET();
  }

  /* Perform 1 turnaround clock cycle */
  if (new_dir == SWDIO_SPI_STATUS_FLOAT) {
    /* Switching to input: read and discard 1 bit */
    swd_spi_read_bits(1);
  } else {
    /* Switching to output: write 1 bit (value doesn't matter) */
    swd_spi_write_bits(0, 1);
  }
}

static uint32_t swdptap_seq_in_spi(size_t clock_cycles) {
  if (!clock_cycles)
    return 0;
  swd_spi_turnaround(SWDIO_SPI_STATUS_FLOAT);
  return swd_spi_read_bits((uint16_t)clock_cycles);
}

static bool swdptap_seq_in_parity_spi(uint32_t *ret, size_t clock_cycles) {
  swd_spi_turnaround(SWDIO_SPI_STATUS_FLOAT);
  uint32_t result = swd_spi_read_bits((uint16_t)clock_cycles);
  uint32_t parity_bit = swd_spi_read_bits(1) & 1U;
  *ret = result;
  return calculate_odd_parity(result) == (bool)parity_bit;
}

static void swdptap_seq_out_spi(uint32_t tms_states, size_t clock_cycles) {
  if (!clock_cycles)
    return;
  swd_spi_turnaround(SWDIO_SPI_STATUS_DRIVE);
  swd_spi_write_bits(tms_states, (uint16_t)clock_cycles);
}

static void swdptap_seq_out_parity_spi(uint32_t tms_states,
                                       size_t clock_cycles) {
  const bool parity = calculate_odd_parity(tms_states);
  swd_spi_turnaround(SWDIO_SPI_STATUS_DRIVE);
  swd_spi_write_bits(tms_states, (uint16_t)clock_cycles);
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
static uint32_t swdptap_seq_in_gpio(size_t clock_cycles)
    __attribute__((optimize(3)));
static bool swdptap_seq_in_parity_gpio(uint32_t *ret, size_t clock_cycles)
    __attribute__((optimize(3)));
static void swdptap_seq_out_gpio(uint32_t tms_states, size_t clock_cycles)
    __attribute__((optimize(3)));
static void swdptap_seq_out_parity_gpio(uint32_t tms_states,
                                        size_t clock_cycles)
    __attribute__((optimize(3)));

/* Get actual delay cycles for current clock divider setting */
static uint32_t swdptap_gpio_delay_cycles(void) {
  return target_clk_divider == UINT32_MAX ? SWD_GPIO_NO_DELAY_CYCLES : target_clk_divider;
}

static void swdptap_turnaround(const swdio_status_t dir) {
  static swdio_status_t olddir = SWDIO_STATUS_FLOAT;
  /* 
   * Reset state on mode switch - if this is first call after GPIO setup, force
   * update 
   */
  static swd_mode_t last_mode = SWD_MODE_NONE;
  if (last_mode != SWD_MODE_GPIO) {
    olddir = SWDIO_STATUS_DRIVE; /* GPIO mode starts with output */
    last_mode = SWD_MODE_GPIO;
    /* Initialize SWDIO_DIR for GPIO mode */
    PIN_SWDIO_DIR_SET(); /* DIR=1 (output) */
  }

  if (dir == olddir)
    return;
  olddir = dir;

  /* Change direction first, then execute turnaround clock */
  if (dir == SWDIO_STATUS_FLOAT) {
    PIN_TMS_SWDIO_SET_IN();
  } else {
    PIN_TMS_SWDIO_SET_OUT();
  }

  /* Turnaround clock cycle after direction change */
  const uint32_t delay_cycles = swdptap_gpio_delay_cycles();
  delay_clk_cycles(delay_cycles);
  PIN_SWCLK_TCK_SET();
  delay_clk_cycles(delay_cycles);
  PIN_SWCLK_TCK_CLR();
}

static uint32_t swdptap_seq_in_gpio(const size_t clock_cycles) {
  uint32_t value = 0;
  const uint32_t delay_cycles = swdptap_gpio_delay_cycles();
  swdptap_turnaround(SWDIO_STATUS_FLOAT);
  if (!clock_cycles)
    return 0;
  for (size_t i = 0; i < clock_cycles; i++) {
    delay_clk_cycles(delay_cycles);
    PIN_SWCLK_TCK_SET();
    value |= ((PIN_TMS_SWDIO_IN() & 1U) << i);
    delay_clk_cycles(delay_cycles);
    PIN_SWCLK_TCK_CLR();
  }
  return value;
}

static bool swdptap_seq_in_parity_gpio(uint32_t *ret, size_t clock_cycles) {
  const uint32_t result = swdptap_seq_in_gpio(clock_cycles);
  const uint32_t delay_cycles = swdptap_gpio_delay_cycles();
  delay_clk_cycles(delay_cycles);
  const uint32_t bit = PIN_TMS_SWDIO_IN();
  PIN_SWCLK_TCK_SET();
  delay_clk_cycles(delay_cycles);
  PIN_SWCLK_TCK_CLR();
  swdptap_turnaround(SWDIO_STATUS_DRIVE);
  *ret = result;
  const bool parity = calculate_odd_parity(result);
  return parity == (bool)bit;
}

static void swdptap_seq_out_gpio(const uint32_t tms_states,
                                 const size_t clock_cycles) {
  uint32_t value = tms_states;
  const uint32_t delay_cycles = swdptap_gpio_delay_cycles();
  swdptap_turnaround(SWDIO_STATUS_DRIVE);
  if (!clock_cycles)
    return;
  for (size_t cycle = clock_cycles; cycle--;) {
    PIN_TMS_SWDIO_OUT(value & 1U);
    delay_clk_cycles(delay_cycles);
    PIN_SWCLK_TCK_SET();
    delay_clk_cycles(delay_cycles);
    value >>= 1U;
    PIN_SWCLK_TCK_CLR();
  }
}

static void swdptap_seq_out_parity_gpio(const uint32_t tms_states,
                                        const size_t clock_cycles) {
  const bool parity = calculate_odd_parity(tms_states);
  const uint32_t delay_cycles = swdptap_gpio_delay_cycles();
  swdptap_seq_out_gpio(tms_states, clock_cycles);
  PIN_TMS_SWDIO_OUT(parity);
  delay_clk_cycles(delay_cycles);
  PIN_SWCLK_TCK_SET();
  delay_clk_cycles(delay_cycles);
  PIN_SWCLK_TCK_CLR();
}

/* =================================================================
 * Public: hardware setup + function-pointer assignment
 * Called from swdptap_init() and from monitor mode-switch command.
 * Does NOT send any SWD sequences.
 * ================================================================= */

void hpm_swd_setup_mode(void) {
  if (hpm_use_spi_mode) {
    swd_spi_pins_setup();
    swd_spi_init_with_freq(hpm_spi_freq_hz);
    swd_proc.seq_in = swdptap_seq_in_spi;
    swd_proc.seq_in_parity = swdptap_seq_in_parity_spi;
    swd_proc.seq_out = swdptap_seq_out_spi;
    swd_proc.seq_out_parity = swdptap_seq_out_parity_spi;
  } else {
    swd_gpio_pins_setup();
    swd_proc.seq_in = swdptap_seq_in_gpio;
    swd_proc.seq_in_parity = swdptap_seq_in_parity_gpio;
    swd_proc.seq_out = swdptap_seq_out_gpio;
    swd_proc.seq_out_parity = swdptap_seq_out_parity_gpio;
  }
}

/* =================================================================
 * swdptap_init: hardware setup (no SWD sequences needed for SWD)
 * ================================================================= */

void swdptap_init(void) {
  extern void aux_serial_enable_pins(void);
  extern void uninit_jtag_tdi_pin(void);

  aux_serial_enable_pins();
  uninit_jtag_tdi_pin();

  hpm_swd_setup_mode();
}
