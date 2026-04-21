/*
 * RTT Debug Commands
 */

#include "general.h"
#include "gdb_packet.h"
#include "target.h"
#include "target_internal.h"
#include "hpm_spi_accel.h"
#include "jtag_port.h"
#include "hpm_clock_drv.h"
#include <string.h>
#include <stdlib.h>

/* External debug functions */
extern uint32_t rtt_get_write_total(void);
extern bool rtt_enabled;
extern volatile bool aux_usb_tx_busy_flag;
extern volatile bool aux_dtr_enable;
extern volatile uint32_t aux_usb_in_callback_count;

static bool cmd_rtt_debug(target_s *target, int argc, const char **argv)
{
    (void)target;
    (void)argc;
    (void)argv;
    
    uint32_t write_total = rtt_get_write_total();
    
    gdb_outf("=== RTT Debug (Direct USB Mode) ===\n");
    gdb_outf("RTT Enabled: %s\n", rtt_enabled ? "YES" : "NO");
    gdb_outf("Total sent to USB: %u bytes\n", write_total);
    gdb_outf("\n=== USB Status ===\n");
    gdb_outf("AUX USB TX Busy: %s\n", aux_usb_tx_busy_flag ? "YES (BLOCKED!)" : "NO");
    gdb_outf("AUX DTR Enabled: %s\n", aux_dtr_enable ? "YES" : "NO");
    gdb_outf("AUX IN Callbacks: %u\n", aux_usb_in_callback_count);
    
    return true;
}

/* ─── Probe mode switch commands ──────────────────────────────────────── */

/*
 * monitor spi [freq_mhz]
 * Switch to SPI-accelerated mode. Optional frequency in MHz (default 20, max 50).
 * Re-run swdp_scan or jtag_scan to apply.
 */
/*
 * monitor bit_mode spi [freq_mhz]
 * monitor bit_mode gpio
 * Switch between SPI accelerated mode and GPIO bitbang mode.
 */
static bool cmd_bit_mode(target_s *t, int argc, const char **argv)
{
    (void)t;
    if (argc < 2) {
        gdb_outf("Usage: bit_mode spi [freq_mhz] | bit_mode gpio\nCurrent: %s mode @ %uMHz\n",
            hpm_use_spi_mode ? "SPI" : "GPIO",
            hpm_spi_freq_hz / 1000000U);
        return true;
    }
    if (strcmp(argv[1], "spi") == 0) {
        hpm_use_spi_mode = true;
        hpm_jtag_setup_mode();
        gdb_outf("SPI mode @ %uMHz. Re-scan to use.\n", hpm_spi_freq_hz / 1000000U);
    } else if (strcmp(argv[1], "gpio") == 0) {
        hpm_use_spi_mode = false;
        hpm_jtag_setup_mode();
        gdb_outf("GPIO bitbang mode. Re-scan to use.\n");
    } else {
        gdb_outf("Unknown mode '%s'. Use: bit_mode spi [freq_mhz] | bit_mode gpio\n", argv[1]);
    }
    return true;
}

/*
 * monitor timing [div <value> | <used_cycles> <cycles_per_cnt>]
 * Display or adjust GPIO bitbang timing calibration parameters.
 * Direct divider setting for empirical calibration.
 */
static bool cmd_timing(target_s *t, int argc, const char **argv)
{
    (void)t;
    
    const uint32_t cpu_freq = clock_get_frequency(clock_cpu0);
    
    if (argc == 1) {
        /* Display current values */
        gdb_outf("GPIO Timing Calibration:\n");
        gdb_outf("  CPU frequency: %u Hz (%.1f MHz)\n", cpu_freq, cpu_freq / 1000000.0f);
        gdb_outf("  Used cycles (overhead): %u\n", hpm_gpio_used_cycles);
        gdb_outf("  Cycles per loop count:  %u\n", hpm_gpio_cycles_per_cnt);
        gdb_outf("  Current divider: %u\n", target_clk_divider == UINT32_MAX ? 0 : target_clk_divider);
        
        if (target_clk_divider != UINT32_MAX) {
            uint32_t cycles = hpm_gpio_used_cycles + 2U * target_clk_divider * hpm_gpio_cycles_per_cnt;
            uint32_t freq = cpu_freq / cycles;
            gdb_outf("  Estimated freq: %u Hz (%.2f MHz)\n", freq, freq / 1000000.0f);
        } else {
            gdb_outf("  No delay mode (max speed)\n");
        }
        
        gdb_outf("\nUsage:\n");
        gdb_outf("  timing                    - Show current values\n");
        gdb_outf("  timing div <value>        - Set divider directly (0=no delay, 1-65535)\n");
        gdb_outf("  timing <used> <per_cnt>   - Set timing parameters (default: 27 2)\n");
        return true;
    }
    
    /* Check if setting divider directly */
    if (argc >= 2 && strcmp(argv[1], "div") == 0) {
        if (argc < 3) {
            gdb_outf("Error: divider value required\n");
            gdb_outf("Usage: timing div <value>  (0=no delay, 1-65535)\n");
            return true;
        }
        
        uint32_t new_div = strtoul(argv[2], NULL, 10);
        if (new_div > 65535U) {
            gdb_outf("Error: divider must be 0-65535\n");
            return true;
        }
        
        target_clk_divider = (new_div == 0) ? UINT32_MAX : new_div;
        
        gdb_outf("Set divider to: %u\n", new_div);
        
        if (new_div > 0) {
            uint32_t cycles = hpm_gpio_used_cycles + 2U * new_div * hpm_gpio_cycles_per_cnt;
            uint32_t freq = cpu_freq / cycles;
            gdb_outf("Estimated freq: %u Hz (%.2f MHz)\n", freq, freq / 1000000.0f);
            gdb_outf("Formula: freq = %u / (%u + 2*%u*%u) = %u / %u\n",
                     cpu_freq, hpm_gpio_used_cycles, new_div, hpm_gpio_cycles_per_cnt,
                     cpu_freq, cycles);
        } else {
            gdb_outf("No delay mode - estimated freq: %u Hz (%.2f MHz)\n",
                     cpu_freq / hpm_gpio_used_cycles, 
                     cpu_freq / hpm_gpio_used_cycles / 1000000.0f);
        }
        
        gdb_outf("\nNow measure actual frequency with logic analyzer!\n");
        return true;
    }
    
    /* Update used_cycles and cycles_per_cnt */
    if (argc < 3) {
        gdb_outf("Error: both parameters required\n");
        gdb_outf("Usage: timing <used_cycles> <cycles_per_cnt>\n");
        return true;
    }
    
    uint32_t new_used = strtoul(argv[1], NULL, 10);
    if (new_used < 10 || new_used > 200) {
        gdb_outf("Error: used_cycles must be 10-200 (typical: 25-30)\n");
        return true;
    }
    
    uint32_t new_cnt = strtoul(argv[2], NULL, 10);
    if (new_cnt < 1 || new_cnt > 50) {
        gdb_outf("Error: cycles_per_cnt must be 1-50 (typical: 2-5)\n");
        return true;
    }
    
    hpm_gpio_used_cycles = new_used;
    hpm_gpio_cycles_per_cnt = new_cnt;
    
    gdb_outf("Updated timing parameters:\n");
    gdb_outf("  Used cycles: %u\n", hpm_gpio_used_cycles);
    gdb_outf("  Cycles/count: %u\n", hpm_gpio_cycles_per_cnt);
    gdb_outf("\nRe-run 'mon fre <freq>' to apply changes.\n");
    
    return true;
}

const command_s platform_cmd_list[] = {
    {"rtt_debug",  cmd_rtt_debug, "Show RTT buffer statistics"},
    {"bit_mode",   cmd_bit_mode,  "Switch probe mode: bit_mode spi | bit_mode gpio"},
    {"timing",     cmd_timing,    "GPIO timing calibration: timing [used_cycles [cycles_per_cnt]]"},
    {NULL, NULL, NULL},
};

