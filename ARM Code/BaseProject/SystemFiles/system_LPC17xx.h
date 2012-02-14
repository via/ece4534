/**************************************************************************//**
 * @file     system_LPC17xx.h
 * @brief    CMSIS Cortex-M3 Device Peripheral Access Layer Header File
 *           for the NXP LPC17xx Device Series
 * @version  V1.02
 * @date     08. September 2009
 *
 * @note
 * Copyright (C) 2009 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M
 * processor based microcontrollers.  This file can be freely distributed
 * within development tools that are supporting such ARM based processors.
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/


#ifndef __SYSTEM_LPC17xx_H
#define __SYSTEM_LPC17xx_H


/* The following are some additional LPC17xx defines for common register settings */
/* Common Register Settings START -----------> */
#define PCONP_PCTIM0    0x00000002
#define PCONP_PCTIM1    0x00000004
#define PCONP_PCUART0   0x00000008
#define PCONP_PCUART1   0x00000010
#define PCONP_PCPWM1    0x00000040
#define PCONP_PCI2C0    0x00000080
#define PCONP_PCSPI     0x00000100
#define PCONP_PCRTC     0x00000200
#define PCONP_PCSSP1    0x00000400
#define PCONP_PCAD      0x00001000
#define PCONP_PCCAN1    0x00002000
#define PCONP_PCCAN2    0x00004000
#define PCONP_PCGPIO    0x00008000
#define PCONP_PCRIT     0x00010000
#define PCONP_PCMCPWM   0x00020000
#define PCONP_PCQEI     0x00040000
#define PCONP_PCI2C1    0x00080000
#define PCONP_PCSSP0    0x00200000
#define PCONP_PCTIM2    0x00400000
#define PCONP_PCTIM3    0x00800000
#define PCONP_PCUART2   0x01000000
#define PCONP_PCUART3   0x02000000
#define PCONP_PCI2C2    0x04000000
#define PCONP_PCI2S     0x08000000
#define PCONP_PCGPDMA   0x20000000
#define PCONP_PCENET    0x40000000
#define PCONP_PCUSB     0x80000000

#define PLLCON_PLLE     0x00000001
#define PLLCON_PLLC     0x00000002
#define PLLCON_MASK     0x00000003

#define PLLCFG_MUL1     0x00000000
#define PLLCFG_MUL2     0x00000001
#define PLLCFG_MUL3     0x00000002
#define PLLCFG_MUL4     0x00000003
#define PLLCFG_MUL5     0x00000004
#define PLLCFG_MUL6     0x00000005
#define PLLCFG_MUL7     0x00000006
#define PLLCFG_MUL8     0x00000007
#define PLLCFG_MUL9     0x00000008
#define PLLCFG_MUL10    0x00000009
#define PLLCFG_MUL11    0x0000000A
#define PLLCFG_MUL12    0x0000000B
#define PLLCFG_MUL13    0x0000000C
#define PLLCFG_MUL14    0x0000000D
#define PLLCFG_MUL15    0x0000000E
#define PLLCFG_MUL16    0x0000000F
#define PLLCFG_MUL17    0x00000010
#define PLLCFG_MUL18    0x00000011
#define PLLCFG_MUL19    0x00000012
#define PLLCFG_MUL20    0x00000013
#define PLLCFG_MUL21    0x00000014
#define PLLCFG_MUL22    0x00000015
#define PLLCFG_MUL23    0x00000016
#define PLLCFG_MUL24    0x00000017
#define PLLCFG_MUL25    0x00000018
#define PLLCFG_MUL26    0x00000019
#define PLLCFG_MUL27    0x0000001A
#define PLLCFG_MUL28    0x0000001B
#define PLLCFG_MUL29    0x0000001C
#define PLLCFG_MUL30    0x0000001D
#define PLLCFG_MUL31    0x0000001E
#define PLLCFG_MUL32    0x0000001F
#define PLLCFG_MUL33    0x00000020
#define PLLCFG_MUL34    0x00000021
#define PLLCFG_MUL35    0x00000022
#define PLLCFG_MUL36    0x00000023

#define PLLCFG_DIV1     0x00000000
#define PLLCFG_DIV2     0x00010000
#define PLLCFG_DIV3     0x00020000
#define PLLCFG_DIV4     0x00030000
#define PLLCFG_DIV5     0x00040000
#define PLLCFG_DIV6     0x00050000
#define PLLCFG_DIV7     0x00060000
#define PLLCFG_DIV8     0x00070000
#define PLLCFG_DIV9     0x00080000
#define PLLCFG_DIV10    0x00090000
#define PLLCFG_MASK	0x00FF7FFF

#define PLLSTAT_MSEL_MASK	0x00007FFF
#define PLLSTAT_NSEL_MASK	0x00FF0000

#define PLLSTAT_PLLE	(1 << 24)
#define PLLSTAT_PLLC	(1 << 25)
#define PLLSTAT_PLOCK	(1 << 26)

#define PLLFEED_FEED1   0x000000AA
#define PLLFEED_FEED2   0x00000055

#define NVIC_IRQ_WDT         0u         // IRQ0,  exception number 16
#define NVIC_IRQ_TIMER0      1u         // IRQ1,  exception number 17
#define NVIC_IRQ_TIMER1      2u         // IRQ2,  exception number 18
#define NVIC_IRQ_TIMER2      3u         // IRQ3,  exception number 19
#define NVIC_IRQ_TIMER3      4u         // IRQ4,  exception number 20
#define NVIC_IRQ_UART0       5u         // IRQ5,  exception number 21
#define NVIC_IRQ_UART1       6u         // IRQ6,  exception number 22
#define NVIC_IRQ_UART2       7u         // IRQ7,  exception number 23
#define NVIC_IRQ_UART3       8u         // IRQ8,  exception number 24
#define NVIC_IRQ_PWM1        9u         // IRQ9,  exception number 25
#define NVIC_IRQ_I2C0        10u        // IRQ10, exception number 26
#define NVIC_IRQ_I2C1        11u        // IRQ11, exception number 27
#define NVIC_IRQ_I2C2        12u        // IRQ12, exception number 28
#define NVIC_IRQ_SPI         13u        // IRQ13, exception number 29
#define NVIC_IRQ_SSP0        14u        // IRQ14, exception number 30
#define NVIC_IRQ_SSP1        15u        // IRQ15, exception number 31
#define NVIC_IRQ_PLL0        16u        // IRQ16, exception number 32
#define NVIC_IRQ_RTC         17u        // IRQ17, exception number 33
#define NVIC_IRQ_EINT0       18u        // IRQ18, exception number 34
#define NVIC_IRQ_EINT1       19u        // IRQ19, exception number 35
#define NVIC_IRQ_EINT2       20u        // IRQ20, exception number 36
#define NVIC_IRQ_EINT3       21u        // IRQ21, exception number 37
#define NVIC_IRQ_ADC         22u        // IRQ22, exception number 38
#define NVIC_IRQ_BOD         23u        // IRQ23, exception number 39
#define NVIC_IRQ_USB         24u        // IRQ24, exception number 40
#define NVIC_IRQ_CAN         25u        // IRQ25, exception number 41
#define NVIC_IRQ_GPDMA       26u        // IRQ26, exception number 42
#define NVIC_IRQ_I2S         27u        // IRQ27, exception number 43
#define NVIC_IRQ_ETHERNET    28u        // IRQ28, exception number 44
#define NVIC_IRQ_RIT         29u        // IRQ29, exception number 45
#define NVIC_IRQ_MCPWM       30u        // IRQ30, exception number 46
#define NVIC_IRQ_QE          31u        // IRQ31, exception number 47
#define NVIC_IRQ_PLL1        32u        // IRQ32, exception number 48
#define NVIC_IRQ_USB_ACT     33u        // IRQ33, exception number 49
#define NVIC_IRQ_CAN_ACT     34u        // IRQ34, exception number 50
/* <----------- Common Register Settings END */


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @addtogroup LPC17xx_System
 * @{
 */


extern uint32_t SystemCoreClock;     /*!< System Clock Frequency (Core Clock)  */


/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
extern void SystemInit (void);

/**
 * Update SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
extern void SystemCoreClockUpdate (void);
#ifdef __cplusplus
}
#endif




/**
 * @}
 */

#endif /* __SYSTEM_LPC17xx_H */
