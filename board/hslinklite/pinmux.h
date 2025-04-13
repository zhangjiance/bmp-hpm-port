
/*
 * Copyright (c) 2025 hpmicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * 
 * Automatically generated by HPM Pinmux Tool at Thu, 03 Apr 2025 23:09:45 GMT
 * 
 * 
 * Note:
 * PY and PZ IOs: if any SOC pin function needs to be routed to these IOs,
 * besides of IOC, PIOC/BIOC needs to be configured SOC_GPIO_X_xx, so that
 * expected SoC function can be enabled on these IOs.
 */

 #ifndef HPM_PINMUX_H
 #define HPM_PINMUX_H
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void init_adc_pins(void);
 void init_py_pins_as_pgpio(void);
 void init_gpio_swj_pins(void);
 void init_jtag_pins(void);
 void init_led_pins(void);
 void init_pwm_pins(void);
 void init_spi1_swd_pins(void);
 void init_spi2_jtag_pins(void);
 void init_uart_pins(UART_Type *ptr);
 void init_usb_pins(USB_Type *ptr) ;
 void init_gptmr_channel_pin(GPTMR_Type *ptr, uint32_t channel, bool as_comp);

 
 #ifdef __cplusplus
 }
 #endif
 #endif /* HPM_PINMUX_H */