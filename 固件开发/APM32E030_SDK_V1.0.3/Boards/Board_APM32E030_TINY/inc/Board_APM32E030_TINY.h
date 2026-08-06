/*!
 * @file        Board_APM32E030_TINY.h
 *
 * @brief       This file contains definitions for APM32E030_TINYBOARD's Leds, push-buttons hardware resources
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

/* Define to prevent recursive inclusion */
#ifndef __BOARD_APM32E030_TINY_H
#define __BOARD_APM32E030_TINY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "apm32e030.h"
#include "apm32e030_gpio.h"
#include "apm32e030_usart.h"
#include "apm32e030_rcm.h"

/* @addtogroup Boards
  @{
*/

/* @addtogroup Board_APM32E030_TINY
  @{
*/

/* @defgroup APM32E030_TINY_Marcos Marcos
  @{
  */
#define LEDn                             2
#define LED2_PIN                         GPIO_PIN_6
#define LED2_GPIO_PORT                   GPIOB
#define LED2_GPIO_CLK                    RCM_AHB_PERIPH_GPIOB

#define LED3_PIN                         GPIO_PIN_7
#define LED3_GPIO_PORT                   GPIOB
#define LED3_GPIO_CLK                    RCM_AHB_PERIPH_GPIOB

#define BUTTONn                          2

/**
 * @brief Key1 push-button
 */
#define KEY1_BUTTON_PIN                   GPIO_PIN_1
#define KEY1_BUTTON_GPIO_PORT             GPIOA
#define KEY1_BUTTON_GPIO_CLK              RCM_AHB_PERIPH_GPIOA
/**
 * @brief Key2 push-button
 */
#define KEY2_BUTTON_PIN                   GPIO_PIN_0
#define KEY2_BUTTON_GPIO_PORT             GPIOA
#define KEY2_BUTTON_GPIO_CLK              RCM_AHB_PERIPH_GPIOA

#define COMn                             2

/**
 * @brief Definition for COM port1, connected to USART1
 */
#define TINY_COM1                        USART1
#define TINY_COM1_CLK                    RCM_APB2_PERIPH_USART1

#define TINY_COM1_TX_PIN                 GPIO_PIN_9
#define TINY_COM1_TX_GPIO_PORT           GPIOA
#define TINY_COM1_TX_GPIO_CLK            RCM_AHB_PERIPH_GPIOA
#define TINY_COM1_TX_SOURCE              GPIO_PIN_SOURCE_9
#define TINY_COM1_TX_AF                  GPIO_AF_PIN1

#define TINY_COM1_RX_PIN                 GPIO_PIN_10
#define TINY_COM1_RX_GPIO_PORT           GPIOA
#define TINY_COM1_RX_GPIO_CLK            RCM_AHB_PERIPH_GPIOA
#define TINY_COM1_RX_SOURCE              GPIO_PIN_SOURCE_10
#define TINY_COM1_RX_AF                  GPIO_AF_PIN1

#define TINY_COM1_IRQn                   USART1_IRQn
/**
 * @brief Definition for COM port2, connected to USART2 (USART2 pins remapped on GPIOD)
 */
#define TINY_COM2                        USART2
#define TINY_COM2_CLK                    RCM_APB1_PERIPH_USART2

#define TINY_COM2_TX_PIN                 GPIO_PIN_2
#define TINY_COM2_TX_GPIO_PORT           GPIOA
#define TINY_COM2_TX_GPIO_CLK            RCM_AHB_PERIPH_GPIOA
#define TINY_COM2_TX_SOURCE              GPIO_PIN_SOURCE_2
#define TINY_COM2_TX_AF                  GPIO_AF_PIN1

#define TINY_COM2_RX_PIN                 GPIO_PIN_3
#define TINY_COM2_RX_GPIO_PORT           GPIOA
#define TINY_COM2_RX_GPIO_CLK            RCM_AHB_PERIPH_GPIOA
#define TINY_COM2_RX_SOURCE              GPIO_PIN_SOURCE_3
#define TINY_COM2_RX_AF                  GPIO_AF_PIN1

#define TINY_COM2_IRQn                   USART2_IRQn

/**@} end of group APM32E030_TINY_Marcos */

/* @defgroup APM32E030_TINY_Emumerations Enumerations
  @{
  */

typedef enum
{
    LED2 = 0,
    LED3 = 1
} LED_T;

typedef enum
{
    BUTTON_KEY1 = 0,
    BUTTON_KEY2 = 1
} BUTTON_T;

typedef enum
{
    BUTTON_MODE_GPIO = 0,
    BUTTON_MODE_EINT = 1
} BUTTONMODE_T;

typedef enum
{
    COM1 = 0,
    COM2 = 1
} COM_T;

/**@} end of group APM32E030_TINY_Emumerations */

/* @defgroup APM32E030_TINY_Structures Structures
  @{
  */

/**@} end of group APM32E030_TINY_Structures */

/* @defgroup APM32E030_TINY_Variables Variables
  @{
  */

/**@} end of group APM32E030_TINY_Variables */

/* @defgroup APM32E030_TINY_Functions Functions
  @{
  */

/* LED */
void APM_TINY_LEDInit(LED_T Led);
void APM_TINY_LEDOn(LED_T Led);
void APM_TINY_LEDOff(LED_T Led);
void APM_TINY_LEDToggle(LED_T Led);

/* Key button */
void APM_TINY_PBInit(BUTTON_T Button, BUTTONMODE_T Button_Mode);
uint32_t APM_TINY_PBGetState(BUTTON_T Button);
void APM_TINY_PB_Led_Isr(void);
/* COM */
void APM_TINY_COMInit(COM_T COM);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_APM32E030_TINY_H */

/**@} end of group APM32E030_TINY_Functions */
/**@} end of group Board_APM32E030_TINY */
/**@} end of group Boards */
