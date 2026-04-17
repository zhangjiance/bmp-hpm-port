/*
 * RTT Debug Commands
 */

#include "general.h"
#include "gdb_packet.h"
#include "target.h"
#include "target_internal.h"

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

const command_s platform_cmd_list[] = {
    {"rtt_debug", cmd_rtt_debug, "Show RTT buffer statistics"},
    {NULL, NULL, NULL},
};
