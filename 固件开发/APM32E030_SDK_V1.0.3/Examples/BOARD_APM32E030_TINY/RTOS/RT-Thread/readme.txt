S/*!
 * @file       readme.txt
 *
 * @brief      This file is routine instruction
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

This example describes how to use RT-Thread for APM32E030.
The IO of LED2 and LED3 is configed to toggle constantly.
The phenomenon of LED2 and LED3 constantly flickered alternately.

&par Software Description

Please unzip the following dependencies in the directory where the package resides beforehand.
  - Middlewares/RealThread/rtthread-nano-master.zip    RT-Thread Software Components

&par Hardware Description

LED2 and LED3

The USART1 is configured as follows:
  - TX:PA9, RX:PA10
  - BaudRate    =   115200
  - Word Length =   USART_WordLength_8b
  - Stop Bit    =   USART_StopBits_1
  - Parity      =   USART_Parity_No
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled

&par Directory contents

  - Source/main.c                       Main program
  - Source/apm32e030_int.c              Interrupt handlers
  - Project/MDK/RTE/RTOS/board.c        RT-Thread firmware functions
  - Project/MDK/RTE/RTOS/rtconfig.h     RT-Thread config functions

&par Hardware and Software environment

  - This example runs on APM32E030 TINY Devices.
