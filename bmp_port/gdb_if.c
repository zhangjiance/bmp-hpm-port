#include "general.h"
#include "gdb_if.h"
#include "cdc_acm.h"


void gdb_if_putchar(const char c, const bool flush)
{
	chry_ringbuffer_write_byte(&g_gdbout, c);
	if(flush||chry_ringbuffer_get_free(&g_gdbout)==0) {
		gdb_if_flush(flush);
	}
}

void gdb_if_flush(const bool force)
{
	uint32_t size;
    uint8_t *buffer;
	/* Flush only if there is data to flush */
	if (chry_ringbuffer_check_empty(&g_gdbout))
		return;
	if(usbtx_idle_flag)
	{
		if (chry_ringbuffer_get_used(&g_gdbout)) {
			/* start first transfer */
			buffer = chry_ringbuffer_linear_read_setup(&g_gdbout, &size);
			usbtx_idle_flag = 0;
			usbd_ep_start_write(0, CDC_IN_EP, buffer, size);
		}
	}
}

char gdb_if_getchar(void)
{
	char c;
	if(usbrx_idle_flag)
	{
		if (chry_ringbuffer_get_free(&g_usbrx) >= CDC_MAX_PACKET_SIZE) {
            usbrx_idle_flag = 0;
            usbd_ep_start_read(0, CDC_OUT_EP, usb_tmpbuffer, CDC_MAX_PACKET_SIZE);
        }
	}
	while(!chry_ringbuffer_read_byte(&g_usbrx,(uint8_t*)&c));
	return c;
}

char gdb_if_getchar_to(const uint32_t timeout)
{
	char c = 0;
	platform_timeout_s receive_timeout;
	platform_timeout_set(&receive_timeout, timeout);

	/* Wait while we need more data or until the timeout expires */
	while (!platform_timeout_is_expired(&receive_timeout))
	{
		if(chry_ringbuffer_read_byte(&g_usbrx,(uint8_t*)&c))
		{
			return c;
		}
	}
	return -1;
}
