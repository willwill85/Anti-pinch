/*!
 * @file        apm32e030_dma.h
 *
 * @brief       This file contains all the functions prototypes for the DMA firmware
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
#ifndef __APM32E030_DMA_H
#define __APM32E030_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "apm32e030.h"

/** @addtogroup APM32E030_StdPeriphDriver
  @{
*/

/** @addtogroup DMA_Driver
  @{
*/

/** @defgroup DMA_Macros Macros
  @{
*/

/**@} end of group DMA_Macros*/

/** @defgroup DMA_Enumerations Enumerations
  @{
*/

/**
 * @brief   DMA data transfer direction
 */
typedef enum
{
    DMA_DIR_PERIPHERAL   = ((uint8_t)0),  /*!< Peripheral to memory */
    DMA_DIR_MEMORY       = ((uint8_t)1),  /*!< Memory to peripheral */
} DMA_DIR_T;

/**
 * @brief   DMA peripheral increment mode
 */
typedef enum
{
    DMA_PERIPHERAL_INC_DISABLE  = ((uint8_t)0),  /*!< Disable peripheral address increment mode */
    DMA_PERIPHERAL_INC_ENABLE   = ((uint8_t)1),  /*!< Enable peripheral address increment mode */
} DMA_PERIPHERAL_INC_T;

/**
 * @brief   DMA memory increment mode
 */
typedef enum
{
    DMA_MEMORY_INC_DISABLE  = ((uint8_t)0),  /*!< Disable memory address increment mode */
    DMA_MEMORY_INC_ENABLE   = ((uint8_t)1),  /*!< Enable memory address increment mode */
} DMA_MEMORY_INC_T;

/**
 * @brief   DMA peripheral data size
 */
typedef enum
{
    DMA_PERIPHERAL_DATASIZE_BYTE       = ((uint8_t)0x00),  /*!< Set peripheral data size to byte */
    DMA_PERIPHERAL_DATASIZE_HALFWORD   = ((uint8_t)0x01),  /*!< Set peripheral data size to half-word */
    DMA_PERIPHERAL_DATASIZE_WORD       = ((uint8_t)0x02),  /*!< Set peripheral data size to word */
} DMA_PERIPHERAL_DATASIZE_T;

/**
 * @brief   DMA memory data size
 */
typedef enum
{
    DMA_MEMORY_DATASIZE_BYTE       = ((uint8_t)0x00),  /*!< Set memory data size to byte */
    DMA_MEMORY_DATASIZE_HALFWORD   = ((uint8_t)0x01),  /*!< Set memory data size to half-word */
    DMA_MEMORY_DATASIZE_WORD       = ((uint8_t)0x02),  /*!< Set memory data size to word */
} DMA_MEMORY_DATASIZE_T;

/**
 * @brief   DMA circular mode
 */
typedef enum
{
    DMA_CIRCULAR_DISABLE  = ((uint8_t)0),  /*!< Disable circular mode */
    DMA_CIRCULAR_ENABLE   = ((uint8_t)1),  /*!< Enable circular mode */
} DMA_CIRCULAR_T;

/**
 * @brief   DMA priority level
 */
typedef enum
{
    DMA_PRIORITY_LEVEL_LOW      = ((uint8_t)0x00),  /*!< Set priority level to low */
    DMA_PRIORITY_LEVEL_MEDIUM   = ((uint8_t)0x01),  /*!< Set priority level to medium */
    DMA_PRIORITY_LEVEL_HIGHT    = ((uint8_t)0x02),  /*!< Set priority level to high */
    DMA_PRIORITY_LEVEL_VERYHIGH = ((uint8_t)0x03),  /*!< Set priority level to very high */
} DMA_PRIORITY_LEVEL_T;

/**
 * @brief   DMA memory to memory
 */
typedef enum
{
    DMA_M2M_DISABLE  = ((uint8_t)0),  /*!< Disable memory to memory mode */
    DMA_M2M_ENABLE   = ((uint8_t)1),  /*!< Enable memory to memory mode */
} DMA_M2M_T;

/**
 * @brief    DMA flag definition
 */
typedef enum
{
    DMA1_FLAG_AL1    = ((uint32_t)0x00000001),  /*!< Channel 1 All flag */
    DMA1_FLAG_TF1    = ((uint32_t)0X00000002),  /*!< Channel 1 Transfer Complete flag */
    DMA1_FLAG_HT1    = ((uint32_t)0X00000004),  /*!< Channel 1 Half Transfer Complete flag */
    DMA1_FLAG_TE1    = ((uint32_t)0X00000008),  /*!< Channel 1 Transfer Error flag */

    DMA1_FLAG_AL2    = ((uint32_t)0x00000010),  /*!< Channel 2 All flag */
    DMA1_FLAG_TF2    = ((uint32_t)0x00000020),  /*!< Channel 2 Transfer Complete flag */
    DMA1_FLAG_HT2    = ((uint32_t)0x00000040),  /*!< Channel 2 Half Transfer Complete flag */
    DMA1_FLAG_TE2    = ((uint32_t)0x00000080),  /*!< Channel 2 Transfer Error flag */

    DMA1_FLAG_AL3    = ((uint32_t)0x00000100),  /*!< Channel 3 All flag */
    DMA1_FLAG_TF3    = ((uint32_t)0x00000200),  /*!< Channel 3 Transfer Complete flag */
    DMA1_FLAG_HT3    = ((uint32_t)0x00000400),  /*!< Channel 3 Half Transfer Complete flag */
    DMA1_FLAG_TE3    = ((uint32_t)0x00000800),  /*!< Channel 3 Transfer Error flag */

    DMA1_FLAG_AL4    = ((uint32_t)0x00001000),  /*!< Channel 4 All flag */
    DMA1_FLAG_TF4    = ((uint32_t)0x00002000),  /*!< Channel 4 Transfer Complete flag */
    DMA1_FLAG_HT4    = ((uint32_t)0x00004000),  /*!< Channel 4 Half Transfer Complete flag */
    DMA1_FLAG_TE4    = ((uint32_t)0x00008000),  /*!< Channel 4 Transfer Error flag */

    DMA1_FLAG_AL5    = ((uint32_t)0x00010000),  /*!< Channel 5 All flag */
    DMA1_FLAG_TF5    = ((uint32_t)0x00020000),  /*!< Channel 5 Transfer Complete flag */
    DMA1_FLAG_HT5    = ((uint32_t)0x00040000),  /*!< Channel 5 Half Transfer Complete flag */
    DMA1_FLAG_TE5    = ((uint32_t)0x00080000),  /*!< Channel 5 Transfer Error flag */
} DMA_FLAG_T;

/**
 * @brief    DMA interrupt flag definition
 */
typedef enum
{
    DMA1_INT_FLAG_AL1 = ((uint32_t)0x00000001),  /*!< Channel 1 All interrupt flag */
    DMA1_INT_FLAG_TF1 = ((uint32_t)0x00000002),  /*!< Channel 1 Transfer Complete interrupt flag */
    DMA1_INT_FLAG_HT1 = ((uint32_t)0x00000004),  /*!< Channel 1 Half Transfer Complete interrupt flag */
    DMA1_INT_FLAG_TE1 = ((uint32_t)0x00000008),  /*!< Channel 1 Transfer Error interrupt flag */

    DMA1_INT_FLAG_AL2 = ((uint32_t)0x00000010),  /*!< Channel 2 All interrupt flag */
    DMA1_INT_FLAG_TF2 = ((uint32_t)0x00000020),  /*!< Channel 2 Transfer Complete interrupt flag */
    DMA1_INT_FLAG_HT2 = ((uint32_t)0x00000040),  /*!< Channel 2 Half Transfer Complete interrupt flag */
    DMA1_INT_FLAG_TE2 = ((uint32_t)0x00000080),  /*!< Channel 2 Transfer Error interrupt flag */

    DMA1_INT_FLAG_AL3 = ((uint32_t)0x00000100),  /*!< Channel 3 All interrupt flag */
    DMA1_INT_FLAG_TF3 = ((uint32_t)0x00000200),  /*!< Channel 3 Transfer Complete interrupt flag */
    DMA1_INT_FLAG_HT3 = ((uint32_t)0x00000400),  /*!< Channel 3 Half Transfer Complete interrupt flag */
    DMA1_INT_FLAG_TE3 = ((uint32_t)0x00000800),  /*!< Channel 3 Transfer Error interrupt flag */

    DMA1_INT_FLAG_AL4 = ((uint32_t)0x00001000),  /*!< Channel 4 All interrupt flag */
    DMA1_INT_FLAG_TF4 = ((uint32_t)0x00002000),  /*!< Channel 4 Transfer Complete interrupt flag */
    DMA1_INT_FLAG_HT4 = ((uint32_t)0x00004000),  /*!< Channel 4 Half Transfer Complete interrupt flag */
    DMA1_INT_FLAG_TE4 = ((uint32_t)0x00008000),  /*!< Channel 4 Transfer Error interrupt flag */

    DMA1_INT_FLAG_AL5 = ((uint32_t)0x00010000),  /*!< Channel 5 All interrupt flag */
    DMA1_INT_FLAG_TF5 = ((uint32_t)0x00020000),  /*!< Channel 5 Transfer Complete interrupt flag */
    DMA1_INT_FLAG_HT5 = ((uint32_t)0x00040000),  /*!< Channel 5 Half Transfer Complete interrupt flag */
    DMA1_INT_FLAG_TE5 = ((uint32_t)0x00080000),  /*!< Channel 5 Transfer Error interrupt flag */
} DMA_INT_FLAG_T;

/**
 * @brief   DMA interrupt source
 */
typedef enum
{
    DMA_INT_TFIE    = BIT1,  /*!< Transfer complete interrupt enable */
    DMA_INT_HTIE    = BIT2,  /*!< Half Transfer interrupt enable */
    DMA_INT_TEIE    = BIT3,  /*!< Transfer error interrupt enable */
} DMA_INT_T;

/**@} end of group DMA_Enumerations */

/** @defgroup DMA_Structures Structures
  @{
*/

/**
 * @brief   DMA Config struct definition
 */
typedef struct
{
    uint32_t                    memoryAddress;        /*!< Specifies the DMA memory base address */
    uint32_t                    peripheralAddress;    /*!< Specifies the DMA peripheral base address */
    DMA_DIR_T                   direction;            /*!< Specifies the DMA data transfer direction */
    uint32_t                    bufferSize;           /*!< Specifies the DMA buffer size */
    DMA_MEMORY_DATASIZE_T       memoryDataSize;       /*!< Specifies the DMA memory data size */
    DMA_PERIPHERAL_DATASIZE_T   peripheralDataSize;   /*!< Specifies the DMA peripheral data size */
    DMA_MEMORY_INC_T            memoryInc;            /*!< Specifies the DMA memory address increment */
    DMA_PERIPHERAL_INC_T        peripheralInc;        /*!< Specifies the DMA peripheral address increment */
    DMA_CIRCULAR_T              circular;             /*!< Specifies the DMA circular mode */
    DMA_PRIORITY_LEVEL_T        priority;             /*!< Specifies the DMA software priority */
    DMA_M2M_T                   memoryTomemory;       /*!< Specifies the DMA memory-to-memory transfer */
} DMA_Config_T;

/**@} end of group DMA_Structures */

/** @defgroup DMA_Variables Variables
  @{
*/

/**@} end of group DMA_Variables*/

/** @defgroup DMA_Functions Functions
  @{
*/

/* Reset and configuration */
void DMA_Reset(DMA_CHANNEL_T* channel);
void DMA_Config(DMA_CHANNEL_T* channel, DMA_Config_T* dmaConfig);
void DMA_ConfigStructInit(DMA_Config_T* dmaConfig);
void DMA_Enable(DMA_CHANNEL_T* channel);
void DMA_Disable(DMA_CHANNEL_T* channel);

/* Data number */
void DMA_SetDataNumber(DMA_CHANNEL_T* channel, uint32_t dataNumber);
uint32_t DMA_ReadDataNumber(DMA_CHANNEL_T* channel);

/* interrupt */
void DMA_EnableInterrupt(DMA_CHANNEL_T* channel, uint32_t interrupt);
void DMA_DisableInterrupt(DMA_CHANNEL_T* channel, uint32_t interrupt);

/* Flag */
uint8_t DMA_ReadStatusFlag(DMA_FLAG_T flag);
void DMA_ClearStatusFlag(uint32_t flag);
uint8_t DMA_ReadIntFlag(DMA_INT_FLAG_T flag);
void DMA_ClearIntFlag(uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif /* __APM32E030_DMA_H */

/**@} end of group DMA_Functions */
/**@} end of group DMA_Driver */
/**@} end of group APM32E030_StdPeriphDriver */
