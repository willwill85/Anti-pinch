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

The program to show how to configure the TMR3 peripheral to generate 4 waveform
with 4 different frequencies (2.5KHz, 5KHz, 25KHz and 50KHz).

  - SystemCoreClock = 72 MHz.
  - TMR3CLK = SystemCoreClock = 72 MHz.Prescaler = 0, TMR counter clock = 72 MHz

  - The TMR3 CC1 register = 72MHz/5000
  - CC1 update rate = 5KHz
  - Channel 1 waveform frequency = 2.5KHz

  - The TMR3 CC2 register = 72MHz/10000
  - CC2 update rate = 10KHz
  - Channel 2 waveform frequency = 5KHz

  - The TMR3 CC3 register = 72MHz/50000
  - CC3 update rate = 50KHz
  - Channel 3 waveform frequency = 25KHz

  - The TMR3 CC4 register = 72MHz/100000
  - CC4 update rate = 100KHz
  - Channel 4 waveform frequency = 50KHz

Display TMR3 waveform by oscilloscope.

&par Hardware Description

  - using TMR3 Channel 1 (PA6) to output waveform
  - using TMR3 Channel 2 (PA7) to output waveform
  - using TMR3 Channel 3 (PB0) to output waveform
  - using TMR3 Channel 4 (PB1) to output waveform

&par Directory contents

  - TMR/TMR_OCToggle/Source/apm32e030_int.c        Interrupt handlers
  - TMR/TMR_OCToggle/Source/main.c                 Main program

&par Hardware and Software environment

  - This example runs on APM32E030 TINY Devices.
