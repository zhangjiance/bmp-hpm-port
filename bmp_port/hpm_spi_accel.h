/*
 * HPM SPI Acceleration mode control header
 * Shared between jtagtap.c, swdptap.c, timing_hpm.c and rtt_debug.c
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Frequency limits for SPI mode */
#define HPM_SPI_FREQ_MAX_HZ      50000000UL   /* 50 MHz hard cap */
#define HPM_SPI_FREQ_MIN_HZ        100000UL   /* 100 kHz */
#define HPM_SPI_FREQ_DEFAULT_HZ  20000000UL   /* 20 MHz default */

/* True = SPI acceleration; False = GPIO bitbang */
extern bool hpm_use_spi_mode;

/* Current SPI target frequency (applies when hpm_use_spi_mode == true) */
extern uint32_t hpm_spi_freq_hz;

/* GPIO bitbang timing calibration parameters */
extern uint32_t hpm_gpio_used_cycles;     /* Base overhead cycles per clock */
extern uint32_t hpm_gpio_cycles_per_cnt;  /* CPU cycles per delay loop iteration */
extern uint32_t target_clk_divider;       /* Current GPIO delay divider */

/*
 * Reconfigure JTAG/SWD pins and update jtag_proc/swd_proc function pointers
 * based on the current value of hpm_use_spi_mode.
 * Does NOT send any JTAG/SWD sequences – safe to call at any time.
 */
void hpm_jtag_setup_mode(void);
void hpm_swd_setup_mode(void);

/*
 * Update SPI clock frequency on both SPI1 and SPI2.
 * Clamps to [HPM_SPI_FREQ_MIN_HZ, HPM_SPI_FREQ_MAX_HZ].
 */
void hpm_spi_set_freq(uint32_t freq_hz);
