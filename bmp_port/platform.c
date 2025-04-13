
#include "general.h"
#include "platform.h"
#include "morse.h"
#include "exception.h"

int platform_hwversion(void)
{
	return 0;
}

void platform_init(void)
{
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

void platform_request_boot(void)
{
}

#pragma GCC diagnostic pop

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