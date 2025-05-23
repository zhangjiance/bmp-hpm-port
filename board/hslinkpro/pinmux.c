/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Note:
 *  PY and PZ IOs: if any SOC pin function needs to be routed to these IOs,
 *  besides of IOC, PIOC/BIOC needs to be configured SOC_GPIO_X_xx, so that
 *  expected SoC function can be enabled on these IOs.
 *
 */
#include "board.h"
#include "pinmux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"

#define TCK_SPI2_CLK IOC_PAD_PB11
#define TDO_SPI2_MISO IOC_PAD_PB12
#define TDI_SPI2_MOSI IOC_PAD_PB13
#define TRST IOC_PAD_PB14
#define SRST IOC_PAD_PB15
#define TMS_O_SPI1_MOSI IOC_PAD_PA29
#define TMS_I_SPI1_MISO IOC_PAD_PA28
#define SWD_DIO_DIR IOC_PAD_PA30

void init_py_pins_as_pgpio(void)
{
    /* Set PY00-PY05 default function to PGPIO */
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = PIOC_PY00_FUNC_CTL_PGPIO_Y_00;
    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = PIOC_PY01_FUNC_CTL_PGPIO_Y_01;
    HPM_PIOC->PAD[IOC_PAD_PY02].FUNC_CTL = PIOC_PY02_FUNC_CTL_PGPIO_Y_02;
    HPM_PIOC->PAD[IOC_PAD_PY03].FUNC_CTL = PIOC_PY03_FUNC_CTL_PGPIO_Y_03;
    HPM_PIOC->PAD[IOC_PAD_PY04].FUNC_CTL = PIOC_PY04_FUNC_CTL_PGPIO_Y_04;
    HPM_PIOC->PAD[IOC_PAD_PY05].FUNC_CTL = PIOC_PY05_FUNC_CTL_PGPIO_Y_05;
}

void init_gpio_swj_pins(void)
{
    HPM_IOC->PAD[TCK_SPI2_CLK].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;
    HPM_IOC->PAD[TCK_SPI2_CLK].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, 11, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOB, 11);
    gpio_write_pin(HPM_FGPIO, GPIO_DO_GPIOB, 11, 1);

    HPM_IOC->PAD[TDO_SPI2_MISO].FUNC_CTL = IOC_PB12_FUNC_CTL_GPIO_B_12;
    HPM_IOC->PAD[TDO_SPI2_MISO].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, 12, gpiom_core0_fast);
    gpio_set_pin_input(HPM_FGPIO, GPIO_OE_GPIOB, 12);

    HPM_IOC->PAD[TDI_SPI2_MOSI].FUNC_CTL = IOC_PB13_FUNC_CTL_GPIO_B_13;
    HPM_IOC->PAD[TDI_SPI2_MOSI].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, 13, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOB, 13);
    gpio_write_pin(HPM_FGPIO, GPIO_DO_GPIOB, 13, 1);

    HPM_IOC->PAD[TRST].FUNC_CTL = IOC_PB14_FUNC_CTL_GPIO_B_14;
    HPM_IOC->PAD[TRST].PAD_CTL =  IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, 14, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOB, 14);
    gpio_write_pin(HPM_FGPIO, GPIO_OE_GPIOB, 14, 1);

    HPM_IOC->PAD[SRST].FUNC_CTL = IOC_PB15_FUNC_CTL_GPIO_B_15;
    HPM_IOC->PAD[SRST].PAD_CTL =  IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, 15, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOB, 15);
    gpio_write_pin(HPM_FGPIO, GPIO_DO_GPIOB, 15, 1);

    HPM_IOC->PAD[TMS_O_SPI1_MOSI].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;
    HPM_IOC->PAD[TMS_O_SPI1_MOSI].PAD_CTL =  IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 29, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOA, 29);
    gpio_write_pin(HPM_FGPIO, GPIO_DO_GPIOA, 29, 0);

    HPM_IOC->PAD[TMS_I_SPI1_MISO].FUNC_CTL = IOC_PA28_FUNC_CTL_GPIO_A_28;
    HPM_IOC->PAD[TMS_I_SPI1_MISO].PAD_CTL =  IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 28, gpiom_core0_fast);
    gpio_set_pin_input(HPM_FGPIO, GPIO_OE_GPIOA, 28);

    HPM_IOC->PAD[SWD_DIO_DIR].FUNC_CTL = IOC_PA30_FUNC_CTL_GPIO_A_30;
    HPM_IOC->PAD[SWD_DIO_DIR].PAD_CTL =  IOC_PAD_PAD_CTL_SR_SET(1)|IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 30, gpiom_core0_fast);
    gpio_set_pin_output(HPM_FGPIO, GPIO_OE_GPIOA, 30);
    gpio_write_pin(HPM_FGPIO, GPIO_DO_GPIOA, 30, 1);
}


void init_uart_pins(UART_Type *ptr)
{
    if (ptr == HPM_UART0) {
        HPM_IOC->PAD[IOC_PAD_PA00].FUNC_CTL = IOC_PA00_FUNC_CTL_UART0_TXD;
        HPM_IOC->PAD[IOC_PAD_PA01].FUNC_CTL = IOC_PA01_FUNC_CTL_UART0_RXD;
    } else if (ptr == HPM_UART3) {
        HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_UART3_TXD;
        HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PB14_FUNC_CTL_UART3_RXD;
    } else {
        ;
    }
}

/* for uart_lin case, need to configure pin as gpio to sent break signal */
void init_uart_pin_as_gpio(UART_Type *ptr)
{
    /* pull-up */
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);

    if (ptr == HPM_UART3) {
        HPM_IOC->PAD[IOC_PAD_PB15].PAD_CTL = pad_ctl;
        HPM_IOC->PAD[IOC_PAD_PB14].PAD_CTL = pad_ctl;
        HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_GPIO_B_15;
        HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PB14_FUNC_CTL_GPIO_B_14;
    }
}

void init_i2c_pins(I2C_Type *ptr)
{
    if (ptr == HPM_I2C2) {
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_I2C2_SCL | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_I2C2_SDA | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB08].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PB09].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    } else if (ptr == HPM_I2C3) {
        HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_I2C3_SCL | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_I2C3_SDA | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB13].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PB12].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    }
}

void init_gpio_pins(void)
{
    /* configure pad setting: pull enable and pull down, schmitt trigger enable */
    /* enable schmitt trigger to eliminate jitter of pin used as button */

    /* Button */
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0) | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL = IOC_PA03_FUNC_CTL_GPIO_A_03;
    HPM_IOC->PAD[IOC_PAD_PA03].PAD_CTL = pad_ctl;
}

void init_spi_pins(SPI_Type *ptr)
{
    if (ptr == HPM_SPI1) {
        HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_SPI1_CS_0;
        HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;
        HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;

        /* set max frequency slew rate(200M) */
        HPM_IOC->PAD[IOC_PAD_PA26].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_PRS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    }
}

void init_spi_pins_with_gpio_as_cs(SPI_Type *ptr)
{
    if (ptr == HPM_SPI1) {
        HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_GPIO_A_26;
        HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;
        HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;

        /* set max frequency slew rate(200M) */
        HPM_IOC->PAD[IOC_PAD_PA26].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_PRS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA29].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    }
}


void init_gptmr_pins(GPTMR_Type *ptr)
{
    if (ptr == HPM_GPTMR0) {
        HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PB10_FUNC_CTL_GPTMR0_COMP_2;
        HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_GPTMR0_COMP_3;
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_GPTMR0_CAPT_1;
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_GPTMR0_COMP_1;
    } else if (ptr == HPM_GPTMR1) {
//        HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL = IOC_PA02_FUNC_CTL_GPTMR1_COMP_1;
    }
}

void init_butn_pins(void)
{
    /* configure pad setting: pull enable and pull up, schmitt trigger enable */
    /* enable schmitt trigger to eliminate jitter of pin used as button */

    /* Button */
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA09].FUNC_CTL = IOC_PA09_FUNC_CTL_GPIO_A_09;
    HPM_IOC->PAD[IOC_PAD_PA09].PAD_CTL = pad_ctl;
}

void init_acmp_pins(void)
{
    /* configure to ACMP_COMP_1(ALT16) function */
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_ACMP_COMP_1;
    /* configure to CMP1_INN4 function */
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

void init_adc_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_A:    ADC0.11/ADC1.11 */
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_B:    ADC0.1 /ADC1.1  */
    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_C:    ADC0.2 /ADC1.2  */
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_D:    ADC0.3 /ADC1.3  */
    HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_IW:   ADC0.4 /ADC1.4  */
    HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_IU:   ADC0.5 /ADC1.5  */
    HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_IV:   ADC0.6 /ADC1.6  */
    HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* Board ID: ADC0.7 /ADC1.7  */
}

void init_adc_bldc_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_IU:   ADC0.5 /ADC1.5  */
    HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;         /* ADC_IV:   ADC0.6 /ADC1.6  */
}

void init_usb_pins(USB_Type *ptr)
{
    if (ptr == HPM_USB0) {
        HPM_IOC->PAD[IOC_PAD_PA24].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

        /* USB0_ID */
        HPM_IOC->PAD[IOC_PAD_PY00].FUNC_CTL = IOC_PY00_FUNC_CTL_USB0_ID;
        /* USB0_OC */
        HPM_IOC->PAD[IOC_PAD_PY01].FUNC_CTL = IOC_PY01_FUNC_CTL_USB0_OC;
        /* USB0_PWR */
        HPM_IOC->PAD[IOC_PAD_PY02].FUNC_CTL = IOC_PY02_FUNC_CTL_USB0_PWR;

        /* PY port IO needs to configure PIOC as well */
        HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = PIOC_PY00_FUNC_CTL_SOC_GPIO_Y_00;
        HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = PIOC_PY01_FUNC_CTL_SOC_GPIO_Y_01;
        HPM_PIOC->PAD[IOC_PAD_PY02].FUNC_CTL = PIOC_PY02_FUNC_CTL_SOC_GPIO_Y_02;
    }
}

void init_led_pins_as_gpio(void)
{
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPIO_A_10;
}

/* for uart_rx_line_status case, need to a gpio pin to sent break signal */
void init_uart_break_signal_pin(void)
{
    HPM_IOC->PAD[IOC_PAD_PA26].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_GPIO_A_26;
}

void init_gptmr_channel_pin(GPTMR_Type *ptr, uint32_t channel, bool as_comp)
{
    if (ptr == HPM_GPTMR0) {
        if (as_comp) {
            switch (channel) {
            case 1:
                HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_GPTMR0_COMP_1;
                break;
            case 2:
                HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PB10_FUNC_CTL_GPTMR0_COMP_2;
                break;
            case 3:
                HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_GPTMR0_COMP_3;
                break;
            default:
                break;
            }
        } else {
            if (channel == 1) {
                HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_GPTMR0_CAPT_1;
            }
        }
    } else if (ptr == HPM_GPTMR1) {
        if ((as_comp == true) && (channel == 3)) {
            HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_GPTMR1_COMP_3;
        }
    }
}
