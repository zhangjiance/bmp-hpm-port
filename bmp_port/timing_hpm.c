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

uint32_t target_clk_divider = UINT32_MAX;

/*
 * HPM5301 Bitbang timing calibration @ 480MHz CPU:
 * 
 * Based on actual measurement: 10MHz setting → 3.7MHz actual
 * 
 * Analysis from disassembly:
 * - Each GPIO write (sw) + fence io,io: ~15-25 CPU cycles
 * - Each bitbang clock has: 3 GPIO writes + 3 fences ≈ 60-80 cycles base overhead
 * - Each delay loop iteration: lw+addi+sw+lw+beqz+j = ~10-15 cycles (with stalls)
 * - Two delay loops per clock cycle
 * 
 * Formula (similar to STM32):
 * cycles_needed = CPU_freq / target_freq
 * divisor = (CPU_freq - USED_CYCLES * target_freq) / 2
 * divider = divisor / (CYCLES_PER_CNT * target_freq)
 */
#define USED_SWD_CYCLES 60U     /* Base overhead: GPIO ops + fences per clock */
#define CYCLES_PER_CNT  25U     /* CPU cycles per delay loop iteration (with stalls) */

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

__attribute__((weak)) void platform_ospeed_update(const uint32_t frequency)
{
	(void)frequency;
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
	uint32_t divisor = cpu_freq - USED_SWD_CYCLES * frequency;
	
	/* If this wrapped to a huge number (frequency too high), use no delay */
	if (divisor >= 0x80000000U) {
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
	target_clk_divider = divisor / (CYCLES_PER_CNT * frequency);
	
	/* Round up if needed */
	if (target_clk_divider * (CYCLES_PER_CNT * frequency) < divisor)
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
	const uint32_t cpu_freq = clock_get_frequency(clock_cpu0);
	
	/* If no delay mode, estimate based on base overhead only */
	if (target_clk_divider == UINT32_MAX)
		return cpu_freq / USED_SWD_CYCLES;
	
	/* Calculate total cycles per clock */
	const uint32_t cycles_per_clock = USED_SWD_CYCLES + 2U * target_clk_divider * CYCLES_PER_CNT;
	
	/* Return actual frequency */
	return cpu_freq / cycles_per_clock;
}
 