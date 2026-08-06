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
#include "apm32e030_misc.h"
#include "apm32e030_eint.h"
#include "apm32e030_rcm.h"

/** @addtogroup Examples
  @{
  */

/** @addtogroup SysTick
  @{
  */

/** @defgroup SysTick_Macros Macros
  @{
*/

/**@} end of group SysTick_Macros */

/** @defgroup SysTick_Enumerations Enumerations
  @{
*/

/**@} end of group SysTick_Enumerations */

/** @defgroup SysTick_Structures Structures
  @{
*/

/**@} end of group SysTick_Structures */

/** @defgroup SysTick_Variables Variables
  @{
*/

/* System tick */
volatile uint32_t sysTick = 0;

/**@} end of group SysTick_Variables */

/** @defgroup SysTick_Functions Functions
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

    APM_TINY_LEDOff(LED2);
    APM_TINY_LEDOn(LED3);

    /* Configure the SysTick to generate a time base equal to 1 ms */
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        while (1);
    }

    while (1)
    {
        if (sysTick >= 1000)
        {
            sysTick = 0;

            APM_TINY_LEDToggle(LED2);
            APM_TINY_LEDToggle(LED3);
        }
    }
}

/**@} end of group SysTick_Functions */
/**@} end of group SysTick */
/**@} end of group Examples */
