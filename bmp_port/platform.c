
#include "general.h"
#include "platform.h"
#include "morse.h"
#include "exception.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_dfu_trigger.h"

int platform_hwversion(void)
{
	return 0;
}

void platform_init(void)
{
	/* Default to JTAG mode on startup - just disable UART to avoid pin conflict */
	/* Note: board_init() already calls init_gpio_swj_pins() for GPIO-based JTAG/SWD */
	extern void uninit_uart2_pins(void);
	uninit_uart2_pins();     /* Ensure UART pins (PA08/PA09) are floating */
}

void platform_nrst_set_val(bool assert)
{
	(void)assert;
}

bool platform_nrst_get_val(void)
{
	return false;
}

const char *platform_target_voltage(void)
{
	return "Unknown";
}

void platform_request_boot(void)
{
	/* Use retention registers (BGPR/PDGO) to persist DFU-entry request across reset */
	hpm_dfu_reboot_to_dfu();
}

#ifdef PLATFORM_HAS_POWER_SWITCH
bool platform_target_get_power(void)
{
	return true;
}

bool platform_target_set_power(const bool power)
{
	return true;
}
#endif

void platform_target_clk_output_enable(bool enable)
{
	(void)enable;
}

bool platform_spi_init(const spi_bus_e bus)
{
	(void)bus;
	return false;
}

bool platform_spi_deinit(const spi_bus_e bus)
{
	(void)bus;
	return false;
}

bool platform_spi_chip_select(const uint8_t device_select)
{
	(void)device_select;
	return false;
}

uint8_t platform_spi_xfer(const spi_bus_e bus, const uint8_t value)
{
	(void)bus;
	return value;
}