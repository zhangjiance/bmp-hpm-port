/*
 * HPM DFU Trigger - retention register based DFU entry
 * Allows APP to request bootloader mode across reset via BGPR/PDGO magic.
 */
#ifndef HPM_DFU_TRIGGER_H
#define HPM_DFU_TRIGGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check and clear DFU trigger magic from retention register.
 * Returns true if APP requested DFU mode before last reset. */
bool hpm_dfu_check_and_clear_trigger(void);

/* Write DFU trigger magic and reset — call from APP to enter DFU mode. */
void hpm_dfu_reboot_to_dfu(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
