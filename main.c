/*********************************************************************************
  *Copyright (C), 2016-2025, jiance.zhang
  *FileName:  main.c
  *Author:  JianCe.Zhang
  *Version:  V1.0
  *Date: 2025-04-03
  *Description: 
  *Function List:
  * 1. void function(void)
  * 2. 
  * 3. 
  * 4. 
  *History:
  * 1. 2025-04-03;JianCe.Zhang;Init Function 
  * 2. 
  * 3. 
  * 4. 
****************************************Includes***********************************/
#include "stdio.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "hpm_clock_drv.h"
#include "usb_config.h"


#include "general.h"
#include "platform.h"
#include "gdb_if.h"
#include "gdb_main.h"
#include "target.h"
#include "exception.h"
#include "gdb_packet.h"
#include "morse.h"
#include "command.h"
#include <stdint.h>
#ifdef ENABLE_RTT
#include "rtt.h"
#endif
#include "jtag_port.h"


#include "rtt.h"


/***************************************Variables***********************************/

/***************************************Functions***********************************/

extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);

static void bmp_poll_loop(void)
{
	SET_IDLE_STATE(false);
	while (gdb_target_running && cur_target) {
		gdb_poll_target();

		// Check again, as `gdb_poll_target()` may
		// alter these variables.
		if (!gdb_target_running || !cur_target)
			break;
		char c = gdb_if_getchar_to(0);
		if (c == '\x03' || c == '\x04')
			target_halt_request(cur_target);
#ifdef ENABLE_RTT
		if (rtt_enabled)
			poll_rtt(cur_target);
#endif
	}

	SET_IDLE_STATE(true);
	const gdb_packet_s *const packet = gdb_packet_receive();
	// If port closed and target detached, stay idle
	if (packet->data[0] != '\x04' || cur_target)
		SET_IDLE_STATE(false);
	gdb_main(packet);
}


int main(void) {
  board_init();
  board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
  intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
  cdc_acm_init(0, (uint32_t)HPM_USB0);
  board_timer_create(50, board_timer_process);

  while (true) {
    TRY(EXCEPTION_ALL) { bmp_poll_loop(); }
    CATCH() {
    default:
      gdb_put_packet_error(0xffU);
      target_list_free();
      gdb_outf("Uncaught exception: %s\n", exception_frame.msg);
      morse("TARGET LOST.", true);
    }
  }

  target_list_free();
  return 0;
}