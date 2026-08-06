/*!
 * @file        apm32e030_pmu.h
 *
 * @brief       This file contains all functions prototype and macros for the PMU peripheral
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

#ifndef __APM32E030_PMU_H
#define __APM32E030_PMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apm32e030.h"

/** @addtogroup APM32E030_StdPeriphDriver
  @{
*/

/** @addtogroup PMU_Driver PMU Driver
  @{
*/

/** @defgroup PMU_Macros Macros
  @{
*/

/**@} end of group PMU_Macros */

/** @defgroup PMU_Enumerations Enumerations
  @{
*/

/**
 * @brief   Wake Up PIN
 */
typedef enum
{
    PMU_WAKEUPPIN_1 = ((uint16_t)0x0100),  /*!< PIN_1 wakeup mcu */
    PMU_WAKEUPPIN_2 = ((uint16_t)0x0200),  /*!< PIN_2 wakeup mcu */
} PMU_WAKEUPPIN_T;

/**
 * @brief   Sleep mode entry
 */
typedef enum
{
    PMU_SLEEPENTRY_WFI = 0x00,  /*!< enter SLEEP mode with WFI instruction */
    PMU_SLEEPENTRY_WFE = 0x01,  /*!< enter SLEEP mode with WFE instruction */
} PMU_SLEEPENTRY_T;

/**
 * @brief   Regulator state is Sleep/Stop mode
 */
typedef enum
{
    PMU_REGULATOR_ON = 0x00,         /*!< STOP mode with regulator ON */
    PMU_REGULATOR_LowPower = 0x01,   /*!< STOP mode with regulator in low power mode */
} PMU_REGULATOR_T;

/**
 * @brief   Stop mode entry
 */
typedef enum
{
    PMU_STOPENTRY_WFI = 0x00,             /*!< Enter STOP mode with WFI instruction */
    PMU_STOPENTRY_WFE = 0x01,             /*!< Enter STOP mode with WFE instruction */
    PMU_STOPENTRY_SLEEPONEXIT = 0x02,     /*!< Enter STOP mode with SLEEPONEXIT instruction */
} PMU_STOPENTRY_T;

/**
 * @brief   Flag
 */
typedef enum
{
    PMU_FLAG_WUPF      = 0x01,  /*!< Wake Up flag */
    PMU_FLAG_STDBYF    = 0x02,  /*!< StandBy flag */
    PMU_FLAG_PVDOF     = 0x04, /*!< Not for APM32E030 devices */
    PMU_FLAG_VREFINTF  = 0x08,  /*!<VREFINT flag */
} PMU_FLAG_T;

/**@} end of group PMU_Enumerations*/

/** @defgroup PMU_Structures Structures
  @{
*/

/**@} end of group PMU_Structures */

/** @defgroup PMU_Variables Variables
  @{
*/

/**@} end of group PMU_Variables */

/** @defgroup PMU_Fuctions Fuctions
  @{
*/

/** Function used to set the PMU configuration to the default reset state */
void PMU_Reset(void);

/** Backup Domain Access function */
void PMU_EnableBackupAccess(void);
void PMU_DisableBackupAccess(void);

/** WakeUp pins configuration functions */
void PMU_EnableWakeUpPin(PMU_WAKEUPPIN_T pin);
void PMU_DisableWakeUpPin(PMU_WAKEUPPIN_T pin);

/** Low Power modes configuration functions */
void PMU_EnterSleepMode(PMU_SLEEPENTRY_T entry);
void PMU_EnterSTOPMode(PMU_REGULATOR_T regulator, PMU_STOPENTRY_T entry);
void PMU_EnterSTANDBYMode(void);

/** Flags management functions */
uint8_t PMU_ReadStatusFlag(PMU_FLAG_T flag);
void PMU_ClearStatusFlag(uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif /* __APM32E030_PMU_H */

/**@} end of group PMU_Functions */
/**@} end of group PMU_Driver */
/**@} end of group APM32E030_StdPeriphDriver */
