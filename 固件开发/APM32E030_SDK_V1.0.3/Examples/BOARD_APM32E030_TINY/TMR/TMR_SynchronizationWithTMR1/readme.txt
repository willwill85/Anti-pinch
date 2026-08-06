/*!
 * @file        readme.txt
 *
 * @brief       This file is routine instruction
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

&par Example Description

This example shows how to synchronize TMR peripherals in cascade mode, two timers
TMR1 and TMR3 are used.

The phenomenon of observe whether the output waveform is correct through
the oscilloscope.

using TMR3 CH1(PA6) to output PWM

&par Directory contents

  - TMR/TMR_SynchronizationWithTMR1/apm32e030_int.c       Interrupt handlers
  - TMR/TMR_SynchronizationWithTMR1/Source/main.c         Main program

&par Hardware and Software environment

  - This example runs on APM32E030 TINY Devices.
