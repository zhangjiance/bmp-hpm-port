/*********************************************************************************
  *Copyright (C), 2016-2025, jiance.zhang
  *FileName:  jtag_port.h
  *Author:  JianCe.Zhang
  *Version:  V1.0
  *Date: 2025-04-06
  *Description: 
  *Function List:
  * 1. void function(void)
  * 2. 
  * 3. 
  * 4. 
  *History:
  * 1. 2025-04-06;JianCe.Zhang;Init Function 
  * 2. 
  * 3. 
  * 4. 
**********************************************************************************/

#ifndef  __jtag_port_H__
#define  __jtag_port_H__

/***************************************Includes***********************************/
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_regs.h"
#include "hpm_gpiom_drv.h"
#include "hpm_soc.h"
#include "hpm_csr_drv.h"
#include "hpm_clock_drv.h"

/***************************************Macros***********************************/

#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE                 
#define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#define JTAG_SPI_BASE               HPM_SPI2
#define JTAG_SPI_BASE_CLOCK_NAME    clock_spi2

#define SWD_SPI_BASE               HPM_SPI1
#define SWD_SPI_BASE_CLOCK_NAME    clock_spi1

#define PIN_GPIOM_BASE    HPM_GPIOM
#define PIN_GPIO          HPM_FGPIO
#define PIN_GPIOM         gpiom_core0_fast

#define PIN_TCK        IOC_PAD_PB11
#define PIN_TMS        IOC_PAD_PA29
#define PIN_TDI        IOC_PAD_PB13
#define PIN_TDO        IOC_PAD_PB12
#define PIN_JTAG_TRST  IOC_PAD_PB14
#define PIN_SRST       IOC_PAD_PB15
#define SWDIO_DIR      IOC_PAD_PA30

/* Precompute GPIO indices for efficiency */
#define TCK_PORT_IDX   GPIO_GET_PORT_INDEX(PIN_TCK)
#define TCK_PIN_IDX    GPIO_GET_PIN_INDEX(PIN_TCK)
#define TCK_PIN_MASK   (1U << TCK_PIN_IDX)
#define TMS_PORT_IDX   GPIO_GET_PORT_INDEX(PIN_TMS)
#define TMS_PIN_IDX    GPIO_GET_PIN_INDEX(PIN_TMS)
#define TMS_PIN_MASK   (1U << TMS_PIN_IDX)
#define TDI_PORT_IDX   GPIO_GET_PORT_INDEX(PIN_TDI)
#define TDI_PIN_IDX    GPIO_GET_PIN_INDEX(PIN_TDI)
#define TDI_PIN_MASK   (1U << TDI_PIN_IDX)
#define TDO_PORT_IDX   GPIO_GET_PORT_INDEX(PIN_TDO)
#define TDO_PIN_IDX    GPIO_GET_PIN_INDEX(PIN_TDO)

#define TRST_PORT_IDX  GPIO_GET_PORT_INDEX(PIN_JTAG_TRST)
#define TRST_PIN_IDX   GPIO_GET_PIN_INDEX(PIN_JTAG_TRST)
#define TRST_PIN_MASK  (1U << TRST_PIN_IDX)
#define SRST_PORT_IDX  GPIO_GET_PORT_INDEX(PIN_SRST)
#define SRST_PIN_IDX   GPIO_GET_PIN_INDEX(PIN_SRST)

#define SWDIO_DIR_PORT_IDX  GPIO_GET_PORT_INDEX(SWDIO_DIR)
#define SWDIO_DIR_PIN_IDX   GPIO_GET_PIN_INDEX(SWDIO_DIR)
#define SWDIO_DIR_PIN_MASK  (1U << SWDIO_DIR_PIN_IDX)

__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void)
{
    PIN_GPIO->DO[TCK_PORT_IDX].SET = TCK_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void)
{
    PIN_GPIO->DO[TCK_PORT_IDX].CLEAR = TCK_PIN_MASK;
    __asm volatile("fence io, io");
}


__STATIC_FORCEINLINE uint32_t PIN_TMS_SWDIO_IN(void)
{
    return (PIN_GPIO->DI[TMS_PORT_IDX].VALUE >> TMS_PIN_IDX) & 1U;
}

__STATIC_FORCEINLINE void PIN_TMS_SWDIO_OUT(uint32_t bit)
{
    if (bit) {
        PIN_GPIO->DO[TMS_PORT_IDX].SET = TMS_PIN_MASK;
    } else {
        PIN_GPIO->DO[TMS_PORT_IDX].CLEAR = TMS_PIN_MASK;
    }
    __asm volatile("fence io, io");
}

/* Optimized versions for constant values - no branch overhead */
__STATIC_FORCEINLINE void PIN_TMS_SWDIO_SET(void)
{
    PIN_GPIO->DO[TMS_PORT_IDX].SET = TMS_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_TMS_SWDIO_CLR(void)
{
    PIN_GPIO->DO[TMS_PORT_IDX].CLEAR = TMS_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_TMS_SWDIO_SET_OUT(void)
{
    PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].SET = SWDIO_DIR_PIN_MASK;
    HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0); /* as gpio*/
    HPM_IOC->PAD[PIN_TMS].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_TMS_SWDIO_SET_IN(void)
{
    PIN_GPIO->DO[SWDIO_DIR_PORT_IDX].CLEAR = SWDIO_DIR_PIN_MASK;
    HPM_IOC->PAD[PIN_TMS].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2);
    HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void)
{
    return (PIN_GPIO->DI[TDI_PORT_IDX].VALUE >> TDI_PIN_IDX) & 1U;
}

__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit)
{
    if (bit) {
        PIN_GPIO->DO[TDI_PORT_IDX].SET = TDI_PIN_MASK;
    } else {
        PIN_GPIO->DO[TDI_PORT_IDX].CLEAR = TDI_PIN_MASK;
    }
    __asm volatile("fence io, io");
}

/* Optimized versions for constant values - no branch overhead */
__STATIC_FORCEINLINE void PIN_TDI_SET(void)
{
    PIN_GPIO->DO[TDI_PORT_IDX].SET = TDI_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_TDI_CLR(void)
{
    PIN_GPIO->DO[TDI_PORT_IDX].CLEAR = TDI_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void)
{
    return (PIN_GPIO->DI[TDO_PORT_IDX].VALUE >> TDO_PIN_IDX) & 1U;
}

__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void)
{
    return (PIN_GPIO->DI[TRST_PORT_IDX].VALUE >> TRST_PIN_IDX) & 1U;
}

__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit)
{
    if (bit) {
        PIN_GPIO->DO[TRST_PORT_IDX].SET = TRST_PIN_MASK;
    } else {
        PIN_GPIO->DO[TRST_PORT_IDX].CLEAR = TRST_PIN_MASK;
    }
    __asm volatile("fence io, io");
}

/* Optimized versions for constant values */
__STATIC_FORCEINLINE void PIN_nTRST_SET(void)
{
    PIN_GPIO->DO[TRST_PORT_IDX].SET = TRST_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_nTRST_CLR(void)
{
    PIN_GPIO->DO[TRST_PORT_IDX].CLEAR = TRST_PIN_MASK;
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
    return (PIN_GPIO->DI[SRST_PORT_IDX].VALUE >> SRST_PIN_IDX) & 1U;
}

__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit)
{
    __asm volatile("fence io, io");
}

__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit)
{
}

__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit)
{
}

__STATIC_INLINE uint32_t TIMESTAMP_GET(void)
{
    uint32_t current_us = (hpm_csr_get_core_cycle() * 1000000) / clock_get_frequency(clock_cpu0);
    return current_us;
}


#endif
/* [] END OF jtag_port.h */