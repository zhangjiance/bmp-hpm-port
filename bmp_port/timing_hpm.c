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
    return ( uint32_t)current_ms;
 }
 
 __attribute__((weak)) void platform_ospeed_update(const uint32_t frequency)
 {
     (void)frequency;
 }
 
 void platform_max_frequency_set(const uint32_t frequency)
 {
 }
 
 uint32_t platform_max_frequency_get(void)
 {
     return 360000000;
 }
 