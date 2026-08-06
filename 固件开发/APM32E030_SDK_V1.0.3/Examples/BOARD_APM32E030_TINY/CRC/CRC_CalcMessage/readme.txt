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

Write the calculated data to CRC DATA register and get the calculated result.
The computer of serial debugging assistant can display the corresponding
information for Computed CRC.

set up ExpectedCRC.
The phenomenon of ComputedCRC compases ExpectedCRC. If ComputedCRC is equal to
ExpectedCRC, LED2 on. If ComputedCRC is not equal to ExpectedCRC, LED3 on.

&par Hardware Description

using USART1(TX:PA9, RX:PA10).
  - USART1 configured as follow:
  - BaudRate    =   115200
  - Word Length =   USART_WordLength_8b
  - Stop Bit    =   USART_StopBits_1
  - Parity      =   USART_Parity_No
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled

&par Software Description

Configure the LED2 and LED3 initialization structures, Configure the USART1
initialization structures,quote calculated CRC program,As the same time,The
computer of serial debugging assistant can display information of ComputedCRC.
ComputedCRC compases ExpectedCRC. Then LED(LED2/LED3)on.

&par Directory contents

  - CRC/CRC_CalcMessage/Source/apm32e030_int.c     Interrupt handlers
  - CRC/CRC_CalcMessage/Source/main.c              Main program

&par Hardware and Software environment

  - This example runs on APM32E030 TINY Devices.
