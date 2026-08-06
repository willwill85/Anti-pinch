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
#include <inttypes.h>
#include "apm32e030_fmc.h"

/** @addtogroup Examples
  @{
  */

/** @addtogroup FMC_WriteOB
  @{
  */

/** @defgroup FMC_WriteOB_Macros Macros
  @{
  */

/* printf function configs to USART1*/
#define DEBUG_USART  USART1

/**@} end of group FMC_WriteOB_Macros */

/** @defgroup FMC_WriteOB_Enumerations Enumerations
  @{
  */

/**@} end of group FMC_WriteOB_Enumerations */

/** @defgroup FMC_WriteOB_Structures Structures
  @{
  */

/**@} end of group FMC_WriteOB_Structures */

/** @defgroup FMC_WriteOB_Variables Variables
  @{
  */

/**@} end of group FMC_WriteOB_Variables */

/** @defgroup FMC_WriteOB_Functions Functions
  @{
  */

/* Delay */
void Delay(void);

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
    uint32_t addr  = 0x1FFFF800;

    APM_TINY_COMInit(COM1);

    FMC_Unlock();

    /* Reset Page data*/
    FMC_EraseOptionByte();

    /* Read Address data*/
    printf("After Erase,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", addr, *(volatile uint32_t*)addr);
    printf("After Erase,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+4), *(volatile uint32_t*)(addr+4));
    printf("After Erase,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+8), *(volatile uint32_t*)(addr+8));
    printf("After Erase,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+12), *(volatile uint32_t*)(addr+12));

    /* Write data in address*/
    FMC_ProgramOptionByteData(addr+4, 0x55);
    FMC_ProgramOptionByteData(addr+6, 0xAA);

    /* Read data again*/
    printf("\r\nAfter Write,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", addr, *(volatile uint32_t*)addr);
    printf("After Write,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+4), *(volatile uint32_t*)(addr+4));
    printf("After Write,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+8), *(volatile uint32_t*)(addr+8));
    printf("After Write,       0x%08" PRIx32 " value = 0x%08" PRIx32 "\r\n", (addr+12), *(volatile uint32_t*)(addr+12));

    /* Lock Flash*/
    FMC_Lock();

    for (;;)
    {
    }
}

/*!
 * @brief       Delay
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void Delay(void)
{
    volatile uint32_t delay = 0xff5;

    while (delay--);
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

/**@} end of group FMC_WriteOB_Functions */
/**@} end of group FMC_WriteOB */
/**@} end of group Examples */
