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
#include "apm32e030_usart.h"
#include "apm32e030_misc.h"

/** @addtogroup Examples
  @{
  */

/** @addtogroup USART_Interrupt
  @{
  */

/** @defgroup USART_Interrupt_Macros Macros
  @{
*/

/* printf function configs to USART1*/
#define DEBUG_USART  USART1

/**@} end of group USART_Interrupt_Macros */

/** @defgroup USART_Interrupt_Enumerations Enumerations
  @{
*/

/**@} end of group USART_Interrupt_Enumerations */

/** @defgroup USART_Interrupt_Structures Structures
  @{
*/

/**@} end of group USART_Interrupt_Structures */

/** @defgroup USART_Interrupt_Variables Variables
  @{
*/

/**@} end of group USART_Interrupt_Variables */

/** @defgroup USART_Interrupt_Functions Functions
  @{
  */

/* USART Write Data */
void USART_Write(USART_T* usart, uint8_t* dat);

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
    GPIO_Config_T gpioConfig;
    USART_Config_T usartConfigStruct;

    /* Enable GPIO clock */
    RCM_EnableAHBPeriphClock(TINY_COM1_TX_GPIO_CLK);

    /* Enable COM1 clock */
    RCM_EnableAPB2PeriphClock(TINY_COM1_CLK);

    /* Connect PXx to USARTx_Tx */
    GPIO_ConfigPinAF(TINY_COM1_TX_GPIO_PORT, TINY_COM1_TX_SOURCE, TINY_COM1_TX_AF);

    /* Connect PXx to USARRX_Rx */
    GPIO_ConfigPinAF(TINY_COM1_RX_GPIO_PORT, TINY_COM1_RX_SOURCE, TINY_COM1_RX_AF);

    /* Configure USART Tx as alternate function push-pull */
    gpioConfig.mode = GPIO_MODE_AF;
    gpioConfig.pin = TINY_COM1_TX_PIN;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.outtype = GPIO_OUT_TYPE_PP;
    gpioConfig.pupd = GPIO_PUPD_PU;
    GPIO_Config(TINY_COM1_TX_GPIO_PORT, &gpioConfig);

    /* Configure USART Rx as input floating */
    gpioConfig.pin  = TINY_COM1_RX_PIN;
    GPIO_Config(TINY_COM1_RX_GPIO_PORT, &gpioConfig);

    /* TINY_USARTs configured as follow: */
    /* BaudRate = 115200 baud */
    usartConfigStruct.baudRate = 115200;
    /* Receive and transmit enabled */
    usartConfigStruct.mode     = USART_MODE_TX_RX;
    /* Hardware flow control disabled (RTS and CTS signals) */
    usartConfigStruct.hardwareFlowCtrl = USART_FLOW_CTRL_NONE;
    /* No parity */
    usartConfigStruct.parity   = USART_PARITY_NONE;
    /* One Stop Bit */
    usartConfigStruct.stopBits =  USART_STOP_BIT_1;
    /* Word Length = 8 Bits */
    usartConfigStruct.wordLength = USART_WORD_LEN_8B;
    /* USART_Config */
    USART_Config(TINY_COM1, &usartConfigStruct);

    /* Enable USART_Interrupt_RXBNEIE */
    USART_EnableInterrupt(TINY_COM1, USART_INT_RXBNEIE);

    NVIC_EnableIRQRequest(TINY_COM1_IRQn, 2);

    /* Enable USART */
    USART_Enable(TINY_COM1);

    /* TINY_COM1 Send data to PC, and you need to open serial assistant to observe */
    USART_Write(TINY_COM1, (uint8_t*)"HELLO APM TINY BOARD\r\n");
    while (1)
    {
    }
}

/*!
 * @brief        serial port tramsimt data
 *
 * @param        pointer to date that need to be sent
 *
 * @retval       None
 *
 * @note
 */
void USART_Write(USART_T* usart, uint8_t* dat)
{
    while (*dat)
    {
        while (USART_ReadStatusFlag(usart, USART_FLAG_TXBE) == RESET);

        USART_TxData(usart, *dat++);
    }
}

/*!
 * @brief        This function handles USART1 RX interrupt Handler
 *
 * @param        None
 *
 * @retval       None
 *
 * @note        This function need to put into  void USART1_IRQHandler(void)
 */
void  USART_Receive_Isr(void)
{
    uint8_t dat;

    if (USART_ReadStatusFlag(TINY_COM1, USART_FLAG_RXBNE) == SET)
    {
        dat = (uint8_t)USART_RxData(TINY_COM1);

        printf("%c", dat);
    }
}

#if defined (__CC_ARM) || defined (__ICCARM__) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       ch:  The characters that need to be send.
*
* @param       *f:  pointer to a FILE that can recording all information
*              needed to control a stream
*
* @retval      The characters that need to be send.
*
* @note
*/
int fputc(int ch, FILE* f)
{
    /* send a byte of data to the serial port */
    USART_TxData(DEBUG_USART, (uint8_t)ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(DEBUG_USART, USART_FLAG_TXBE) == RESET);

    return (ch);
}

#elif defined (__GNUC__)

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       ch:  The characters that need to be send.
*
* @retval      The characters that need to be send.
*
* @note
*/
int __io_putchar(int ch)
{
    /* send a byte of data to the serial port */
    USART_TxData(DEBUG_USART, ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(DEBUG_USART, USART_FLAG_TXBE) == RESET);

    return ch;
}

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       file:  Meaningless in this function.
*
* @param       *ptr:  Buffer pointer for data to be sent.
*
* @param       len:  Length of data to be sent.
*
* @retval      The characters that need to be send.
*
* @note
*/
int _write(int file, char* ptr, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        __io_putchar(*ptr++);
    }

    return len;
}

#else
#warning Not supported compiler type
#endif

/**@} end of group USART_Interrupt_Functions */
/**@} end of group USART_Interrupt */
/**@} end of group Examples */
