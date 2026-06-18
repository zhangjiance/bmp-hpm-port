/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2015 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2023 1BitSquared <info@1bitsquared.com>
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
#include "general.h"
#include "platform.h"
#include "board.h"
#include "hpm_spi_accel.h"
#include "hpm_spi_drv.h"
#include "jtag_port.h"
#include <limits.h>

uint32_t target_clk_divider = UINT32_MAX ;

/* Global mode flag and SPI frequency */
bool     hpm_use_spi_mode = false;
uint32_t hpm_spi_freq_hz  = HPM_SPI_FREQ_DEFAULT_HZ;

/*
 * HPM5361 GPIO Bitbang Timing Calibration @ 480MHz CPU
 * 
 * Based on empirical measurement with RISC-V assembly delay loop:
 * 
 * Measured Data:
 * div=0:  18.0 MHz (no delay)
 * div=1:  15.0 MHz
 * div=2:  14.0 MHz
 * div=10:  7.0 MHz
 * div=50:  2.0 MHz
 * div=100: 1.1 MHz
 * div=200: 579 KHz
 * 
 * Analysis:
 * - Base overhead (GPIO ops + fences): 480M/18M ≈ 27 CPU cycles
 * - Each delay iteration (addi + bnez): ~2 CPU cycles (RISC-V assembly optimized)
 * - Each SWD clock has 2 delay calls: total = 27 + 2*divider*2 = 27 + 4*divider
 * 
 * Formula: freq = CPU_freq / (27 + 2*divider*2)
 * Reverse: divider = (CPU_freq/freq - 27) / 4
 * 
 * Verification:
 * div=1:  480M/(27+4)   = 15.48 MHz ✓
 * div=10: 480M/(27+40)  = 7.16 MHz  ✓
 * div=50: 480M/(27+200) = 2.11 MHz  ✓
 *
 * Adjustable via 'mon timing <used_cycles> <cycles_per_cnt>' or 'mon timing div <value>'
 */
uint32_t hpm_gpio_used_cycles = 27U;    /* Base overhead: GPIO ops + fences per clock */
uint32_t hpm_gpio_cycles_per_cnt = 2U;  /* CPU cycles per delay loop iteration (assembly optimized) */

void platform_timing_init(void)
{
}

void platform_delay(uint32_t ms)
{
	board_delay_ms(ms);
}

void sys_tick_handler(void)
{
}

uint32_t platform_time_ms(void)
{
	uint64_t current_ms = (hpm_csr_get_core_cycle() * 1000) / clock_get_frequency(clock_cpu0);
	return (uint32_t)current_ms;
}

void hpm_spi_set_freq(uint32_t freq_hz)
{
        if (freq_hz > HPM_SPI_FREQ_MAX_HZ)
                freq_hz = HPM_SPI_FREQ_MAX_HZ;
        if (freq_hz < HPM_SPI_FREQ_MIN_HZ)
                freq_hz = HPM_SPI_FREQ_MIN_HZ;
        hpm_spi_freq_hz = freq_hz;

        /* Update SPI2 (JTAG) timing */
        {
                spi_timing_config_t t = {0};
                spi_master_get_default_timing_config(&t);
                t.master_config.cs2sclk            = spi_cs2sclk_half_sclk_1;
                t.master_config.csht               = spi_csht_half_sclk_1;
                t.master_config.clk_src_freq_in_hz = clock_get_frequency(JTAG_SPI_BASE_CLOCK_NAME);
                t.master_config.sclk_freq_in_hz    = freq_hz;
                if (status_success != spi_master_timing_init(JTAG_SPI_BASE, &t))
                        spi_master_set_sclk_div(JTAG_SPI_BASE, 0xFF);
        }

        /* Update SPI1 (SWD) timing */
        {
                spi_timing_config_t t = {0};
                spi_master_get_default_timing_config(&t);
                t.master_config.cs2sclk            = spi_cs2sclk_half_sclk_1;
                t.master_config.csht               = spi_csht_half_sclk_1;
                t.master_config.clk_src_freq_in_hz = clock_get_frequency(SWD_SPI_BASE_CLOCK_NAME);
                t.master_config.sclk_freq_in_hz    = freq_hz;
                if (status_success != spi_master_timing_init(SWD_SPI_BASE, &t))
                        spi_master_set_sclk_div(SWD_SPI_BASE, 0xFF);
        }
}

/*
 * Called from platform_max_frequency_set before the GPIO divider calculation.
 * In SPI mode: update SPI clock frequency.
 * In GPIO mode: no-op (divider calculation handles it).
 */
void platform_ospeed_update(const uint32_t frequency)
{
        if (hpm_use_spi_mode && frequency)
                hpm_spi_set_freq(frequency);
}

/*
 * Set the maximum JTAG/SWD frequency
 * 
 * Uses STM32-style formula for better accuracy:
 * divisor = (CPU_freq - USED_CYCLES * target_freq) / 2
 * divider = divisor / (CYCLES_PER_CNT * target_freq)
 * 
 * Example @ 480MHz CPU:
 * - 10MHz: divisor=(480M - 60*10M)/2 = -60M/2 (wraps!) → use no delay
 * - 4MHz:  divisor=(480M - 60*4M)/2 = 240M/2 = 120M
 *          divider=120M/(25*4M) = 120M/100M = 1
 *          Actual: 60 + 2*1*25 = 110 cycles → 4.36MHz
 * - 2MHz:  divisor=(480M - 60*2M)/2 = 180M
 *          divider=180M/(25*2M) = 180M/50M = 3
 *          Actual: 60 + 2*3*25 = 210 cycles → 2.29MHz
 * - 1MHz:  divisor=(480M - 60*1M)/2 = 210M
 *          divider=210M/(25*1M) = 210M/25M = 8
 *          Actual: 60 + 2*8*25 = 460 cycles → 1.04MHz
 */
void platform_max_frequency_set(const uint32_t frequency)
{
	platform_ospeed_update(frequency);
	
	const uint32_t cpu_freq = clock_get_frequency(clock_cpu0);
	
	/* Calculate: divisor = CPU_freq - USED_CYCLES * target_freq */
	uint32_t divisor = cpu_freq - hpm_gpio_used_cycles * frequency;
	
	/* If this wrapped to a huge number (frequency too high), use no delay */
	if (divisor >= 0x20000000U) {
		target_clk_divider = UINT32_MAX;
		return;
	}
	
	/* If frequency is 0, use slowest speed */
	if (!frequency) {
		target_clk_divider = UINT32_MAX - 1U;
		return;
	}
	
	/* Divide by 2 (two delay loops per clock) */
	divisor /= 2U;
	
	/* Calculate divider = divisor / (CYCLES_PER_CNT * frequency) */
	target_clk_divider = divisor / (hpm_gpio_cycles_per_cnt * frequency);
	
	/* Round up if needed */
	if (target_clk_divider * (hpm_gpio_cycles_per_cnt * frequency) < divisor)
		++target_clk_divider;
}

/*
 * Get the currently configured JTAG/SWD frequency
 * 
 * Reverse calculation:
 * cycles_per_clock = USED_CYCLES + 2 * divider * CYCLES_PER_CNT
 * frequency = CPU_freq / cycles_per_clock
 */
uint32_t platform_max_frequency_get(void)
{
	/* In SPI mode, return the configured SPI clock frequency */
	if (hpm_use_spi_mode)
		return hpm_spi_freq_hz;

	const uint32_t cpu_freq = clock_get_frequency(clock_cpu0);

	/* If no delay mode, estimate based on base overhead only */
	if (target_clk_divider == UINT32_MAX)
		return cpu_freq / hpm_gpio_used_cycles;
	
	/* Calculate total cycles per clock */
	const uint32_t cycles_per_clock = hpm_gpio_used_cycles + 2U * target_clk_divider * hpm_gpio_cycles_per_cnt;
	
	/* Return actual frequency */
	return cpu_freq / cycles_per_clock;
}

/* Dynamic clock selection algorithm from CherryDAP HSLink-Pro.
 * Enumerate all available clock sources (clock_source_general_source_end),
 * find the PLL/divider combination that produces frequency closest to target.
 * 
 * Parameters:
 *   clock_name: Target clock (e.g., clock_spi1, clock_spi2)
 *   freq_hz: Target frequency in Hz
 *   best_clk_src: Output - selected clock source
 *   best_div: Output - selected divider (1-256)
 * 
 * Returns: Actual frequency achieved in Hz
 */
uint32_t select_optimal_clock_config(clock_name_t clock_name, uint32_t freq_hz, 
                                      clk_src_t *best_clk_src, uint32_t *best_div) {
  uint32_t freq_list[clock_source_general_source_end] = {0};
  uint32_t pll_freq, div;
  int min_diff_freq = INT_MAX;
  int current_diff_freq;
  uint32_t best_freq = 0;
  
  /* Enumerate all general clock sources to build frequency list */
  for (clock_source_t src = clock_source_osc0_clk0; src < clock_source_general_source_end; src++) {
    /* Convert clock_source_t to clk_src_t (they have same enum values) */
    clk_src_t clk_src = (clk_src_t)src;
    
    /* Probe PLL frequency by temporarily setting divider=1 */
    clock_set_source_divider(clock_name, clk_src, 1);
    pll_freq = clock_get_frequency(clock_name);
    
    if (pll_freq == 0)
      continue; /* Skip disabled PLLs */
    
    /* Calculate divider needed to reach target frequency (range: 1-256) */
    div = pll_freq / freq_hz;
    if (div > 0 && div <= 256) {
      freq_list[src] = pll_freq / div;
    }
  }
  
  /* Find the frequency with minimum error */
  for (int i = 0; i < clock_source_general_source_end; i++) {
    if (freq_list[i] == 0)
      continue;
    
    current_diff_freq = (freq_list[i] > freq_hz) ? 
                        (freq_list[i] - freq_hz) : (freq_hz - freq_list[i]);
    
    if (current_diff_freq < min_diff_freq) {
      min_diff_freq = current_diff_freq;
      best_freq = freq_list[i];
    }
  }
  
  /* Find which source produces the best frequency */
  *best_clk_src = clk_src_pll0_clk2; /* Default fallback */
  *best_div = 1;
  
  for (int i = 0; i < clock_source_general_source_end; i++) {
    if (best_freq == freq_list[i]) {
      *best_clk_src = (clk_src_t)i;
      
      /* Recalculate PLL frequency for this source */
      clock_set_source_divider(clock_name, *best_clk_src, 1);
      pll_freq = clock_get_frequency(clock_name);
      *best_div = pll_freq / best_freq;
      break;
    }
  }
  
  return best_freq;
}
 