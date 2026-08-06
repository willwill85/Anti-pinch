# AntiPinch_MDK

APM32E030K8T6 firmware for the Anti-pinch board.

- LED: PB5, active low, continuously on after reset.
- ADC0..ADC9: PA0..PA7, PB0, PB1.
- RS485/UART1: PA9 TX, PA10 RX, PA8 DE and /RE.
- UART format: 115200 8-N-1. One filtered ADC line is sent every second.
- Clock: internal 8 MHz RC oscillator, because the board has no HSE crystal.

The project uses `APM32E030_SDK_V1.0.3` and the Geehy APM32E030 device pack.

Build the MDK project to create `Out\\APM32E030\\AntiPinch_MDK.hex`, then run
`flash_jlink.bat` from this directory to program it through SWD.
