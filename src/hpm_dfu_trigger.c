/*
 * HPM DFU Trigger Implementation
 *
 * Uses retention registers (BGPR/PDGO) to persist a DFU-entry request
 * across reset. Call hpm_dfu_reboot_to_dfu() from the APP to request
 * DFU mode on next boot.
 */
#include "hpm_dfu_trigger.h"

#include <stdio.h>
#include "board.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_ppor_drv.h"
#ifdef HPM_BCFG_BASE
#include "hpm_bgpr_drv.h"
#endif
#ifdef HPM_PDGO_BASE
#include "hpm_pdgo_drv.h"
#endif

#define DFU_TRIGGER_MAGIC      (0x55464455UL)
#define DFU_TRIGGER_BGPR_INDEX (0U)

bool hpm_dfu_check_and_clear_trigger(void)
{
    bool triggered = false;

#ifdef HPM_BCFG_BASE
    uint32_t val = 0;
    if (bgpr_read32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, &val) == status_success) {
        if (val == DFU_TRIGGER_MAGIC) {
            (void)bgpr_write32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, 0);
            triggered = true;
        }
    }
#endif
#ifdef HPM_PDGO_BASE
    if (pdgo_is_retention_mode_enabled(HPM_PDGO)) {
        if (pdgo_read_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX) == DFU_TRIGGER_MAGIC) {
            pdgo_write_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX, 0);
            triggered = true;
        }
    }
#endif

    return triggered;
}

void hpm_dfu_reboot_to_dfu(void)
{
#ifdef HPM_BCFG_BASE
    (void)bgpr_write32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, DFU_TRIGGER_MAGIC);
#endif
#ifdef HPM_PDGO_BASE
    if (!pdgo_is_retention_mode_enabled(HPM_PDGO)) {
        pdgo_enable_retention_mode(HPM_PDGO);
    }
    pdgo_write_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX, DFU_TRIGGER_MAGIC);
#endif
    printf("dfu trigger received, reboot to DFU bootloader...\r\n");

    ppor_reset_mask_set_source_enable(HPM_PPOR, ppor_reset_software);
    ppor_sw_reset(HPM_PPOR, 24);
    while (1) {
    }
}
