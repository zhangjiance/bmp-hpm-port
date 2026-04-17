/*
 * RTT Debug Commands
 */

#include "general.h"
#include "gdb_packet.h"
#include "target.h"
#include "target_internal.h"

/* External debug functions */
extern uint32_t rtt_get_write_total(void);
extern uint32_t rtt_get_read_total(void);
extern uint32_t rtt_get_available(void);
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
    uint32_t read_total = rtt_get_read_total();
    uint32_t available = rtt_get_available();
    
    gdb_outf("=== RTT Buffer Debug ===\n");
    gdb_outf("RTT Enabled: %s\n", rtt_enabled ? "YES" : "NO");
    gdb_outf("Total written to buffer: %u bytes\n", write_total);
    gdb_outf("Total read from buffer:  %u bytes\n", read_total);
    gdb_outf("Currently available:     %u bytes\n", available);
    gdb_outf("Lost data: %u bytes\n", write_total > read_total ? write_total - read_total : 0);
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
