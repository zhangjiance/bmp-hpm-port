/*
 * RTT Debug Commands
 */

#include "general.h"
#include "gdb_packet.h"
#include "target.h"
#include "target_internal.h"
#include "hpm_spi_accel.h"

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

const command_s platform_cmd_list[] = {
    {"rtt_debug",  cmd_rtt_debug, "Show RTT buffer statistics"},
    {"bit_mode",   cmd_bit_mode,  "Switch probe mode: bit_mode spi | bit_mode gpio"},
    {NULL, NULL, NULL},
};

