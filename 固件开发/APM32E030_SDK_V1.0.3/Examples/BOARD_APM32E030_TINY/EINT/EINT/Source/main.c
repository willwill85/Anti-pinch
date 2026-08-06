/*!
 * @file        main.c
 *
 * @brief       Main program body
 *
 * @version     V1.0.2
 *
 * @date        2025-05-15
 *
 * @attention
 *
 *  Copyright (C) 2024-2025 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Includes */
#include "Board.h"
#include "stdio.h"
#include "apm32e030_gpio.h"
#include "apm32e030_eint.h"
#include "apm32e030_misc.h"
#include "apm32e030_syscfg.h"

/** @addtogroup Examples
  @{
  */

/** @addtogroup EINT
  @{
  */

/** @defgroup EINT_Macros Macros
  @{
  */

/* define KEY1,KEY2 EXTI LINE*/
#define KEY1_BUTTON_EXTI_LINE  EINT_LINE1
#define KEY2_BUTTON_EXTI_LINE  EINT_LINE0

/** @defgroup EINT_Enumerations Enumerations
  @{
  */

/**@} end of group EINT_Enumerations */

/** @defgroup EINT_Structures Structures
  @{
  */

/**@} end of group EINT_Structures */

/** @defgroup EINT_Variables Variables
  @{
  */

/**@} end of group EINT_Variables */

/** @defgroup EINT_Functions Functions
  @{
  */

/*!
 * @brief       Main program
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
int main(void)
{
    APM_TINY_LEDInit(LED2);
    APM_TINY_LEDInit(LED3);

    GPIO_Config_T gpioConfig;
    EINT_Config_T eintConfig;

    /* Enable the BUTTON Clock */
    RCM_EnableAHBPeriphClock(KEY1_BUTTON_GPIO_CLK);
    RCM_EnableAHBPeriphClock(KEY2_BUTTON_GPIO_CLK);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SYSCFG);

    /* Configure Button pin as input floating */
    gpioConfig.mode = GPIO_MODE_IN;
    gpioConfig.pupd = GPIO_PUPD_PU;
    gpioConfig.pin = KEY1_BUTTON_PIN;
    GPIO_Config(KEY1_BUTTON_GPIO_PORT, &gpioConfig);

    gpioConfig.pin = KEY2_BUTTON_PIN;
    GPIO_Config(KEY2_BUTTON_GPIO_PORT, &gpioConfig);

    /* Configure GPIO pin used as EINT Line */
    SYSCFG_EINTLine(SYSCFG_PORT_GPIOA, SYSCFG_PIN_1);
    SYSCFG_EINTLine(SYSCFG_PORT_GPIOA, SYSCFG_PIN_0);

    /* Configure Button exit line */
    eintConfig.line    =  KEY1_BUTTON_EXTI_LINE;
    eintConfig.lineCmd =  ENABLE;
    eintConfig.mode    =  EINT_MODE_INTERRUPT;
    eintConfig.trigger =  EINT_TRIGGER_FALLING;
    EINT_Config(&eintConfig);

    eintConfig.line    =  KEY2_BUTTON_EXTI_LINE;
    EINT_Config(&eintConfig);

    /* Configure NVIC_IRQRequest */
    NVIC_EnableIRQRequest(EINT0_1_IRQn, 0x0f);

    while (1);
}

/**
 * @brief       The interrupt will happen when the button is press ,
 *              and the Led will be on
 *
 * @param       None
 *
 * @retval      None
 *
 * @note        This function should put into EINT0_1_IRQHandler() in apm32e030_int.c
 */
void APM_TINY_PB_LED_Isr(void)
{

    if (EINT_ReadStatusFlag(KEY1_BUTTON_EXTI_LINE) == SET)
    {
        /* The interrupt flag bit must be cleared first. */
        EINT_ClearStatusFlag(KEY1_BUTTON_EXTI_LINE);
        APM_TINY_LEDToggle(LED2);
    }

    if (EINT_ReadStatusFlag(KEY2_BUTTON_EXTI_LINE) == SET)
    {
        /* The interrupt flag bit must be cleared first. */
        EINT_ClearStatusFlag(KEY2_BUTTON_EXTI_LINE);
        APM_TINY_LEDToggle(LED3);
    }
}

/**@} end of group EINT_Functions */
/**@} end of group EINT */
/**@} end of group Examples */
