# AntiPinch_MDK

APM32E030K8T6 firmware for the Anti-pinch board.

## Hardware

- LED: PB5, active low, driven as TMR3_CH2 PWM (AF1).
- ADC0..ADC9: PA0..PA7, PB0, PB1.
- RS485/UART1: PA9 TX, PA10 RX, PA8 DE and /RE. 115200 8-N-1.
- Clock: internal 8 MHz RC oscillator, because the board has no HSE crystal.

## Behaviour

**LED**

| Phase | Pattern |
| --- | --- |
| Reset | 5 blinks, 150 ms on / 150 ms off |
| Normal | breathing, 3.2 s cycle, 1 kHz PWM |
| Pinch detected | fast blink, 150 ms period |

**Signal chain**, per channel, every 10 ms:

1. Invert the raw DMA sample: `v = 4096 − adc`. Everything downstream uses `v`.
2. 5-point median on `v` (spike rejection).
3. Scalar Kalman filter, Q8 fixed point, steady-state gain ≈ 0.15.

The divider is `VCC — 5k — node — film — GND` with the node on the ADC, so the
film resistance is `R = 5k × adc / (4096 − adc)` and the denominator is exactly
`v`. Pressing the film lowers `R`, which lowers `adc` and so raises `v`. Unloaded,
the board reads `adc ≈ 4093`, i.e. `v ≈ 3` and `R ≈ 6.8 MΩ`.

**Baseline `xn`** — the Kalman output is pushed into a 100-entry queue once every
3 s (300 s = 5 minutes of history). `xn` is the median of the *non-zero* entries;
zeros are treated as invalid samples. Medians are recomputed one channel per main
loop pass so the 10 sorts never stall the loop at once.

**Pinch detection** — a channel trips when `v` rises more than
`PINCH_TRIP_DELTA` (200) above `xn`, and clears once it falls back within
`PINCH_CLEAR_DELTA` (120). The gap is hysteresis, so a signal sitting on the
threshold does not chatter the LED and the bus. An absolute delta is used instead
of a ratio because `xn` sits near zero when unloaded, which makes a ratio
meaningless. With the unloaded baseline `v ≈ 3`, a trip at `v = 203` corresponds
to the film falling to roughly 96 kΩ — retune `PINCH_TRIP_DELTA` against real
film readings. Detection is suppressed for the first 10 s after a cold start
(sampling and queueing still run normally), and a channel whose baseline is still
0 (no median computed yet) is skipped.

On every trip and clear the event is printed to RTT. When the module as a whole
changes between normal and pinched, the LED mode switches and one detect-state
frame is pushed onto RS485.

**RTT** — every value is printed as film resistance in kΩ, never as an ADC code:

- every 100 ms, one line with all 10 channel resistances plus the pinch bitmask;
- one baseline line each time the 10 medians finish refreshing (every 3 s);
- one line per trip/clear event with the channel, its resistance and `xn`.

All debug output goes to RTT; nothing is printed on RS485, which carries protocol
frames only.

## RS485 protocol

Implements 防夹模块通信协议 V0.1 (`rs485_proto.c`). Frame layout, little endian:

    SOF(1) STATUS(1) FRAME_ID(1) SRC_ID(1) DST_ID(1) DATA_LEN(2)
    HEAD_CKSUM(2) DATA(DATA_LEN) DATA_CKSUM(2)

Both checksums are CRC-16/MODBUS; the head checksum covers SOF..DATA_LEN, the
data checksum covers the data field only. Module ID is 0x30. Supported commands:

| Command | Request DATA | Reply DATA |
| --- | --- | --- |
| 0x01 查询检测状态 | `01` | `01 <00 normal / 01 abnormal>` |
| 0xF0 查询固件版本 | `F0` | `F0 <major> <minor> <patch>` |

Three things the PDF does not specify, resolved here and worth confirming:

- **Host ID** — the spec only names the module (0x30). `RS485_HOST_ID` is 0x01.
- **Reply FRAME_ID** — replies echo the request's FRAME_ID.
- **Event push** — the spec is strictly poll/response, with no unsolicited
  message. To report a pinch without waiting to be polled, the firmware also
  sends a 0x01 detect-state frame with `STATUS = 1` (request) on each state
  change. Drop `Rs485_ReportDetectState()` if the host must not see this.

## Building with MDK (reference build)

Build the MDK project to create `Out\APM32E030\AntiPinch_MDK.hex`, then run
`flash_jlink.bat` from this directory to program it through SWD.

## Building with GCC (macOS / Linux)

`Makefile` builds the same sources with `arm-none-eabi-gcc`. It needs a full Arm
GNU toolchain — Homebrew's `arm-none-eabi-gcc` ships without newlib and will
fail on `stdint.h`. Point `TOOLCHAIN_BIN` at a complete toolchain (it defaults
to `~/opt/arm-gnu-toolchain/bin`).

    make                # -> Build/AntiPinch_MDK.hex
    make flash          # program over SWD with JLinkExe
    make gdbserver      # J-Link GDB server, also hosts RTT on port 19021
    make rtt            # attach to that RTT terminal (needs gdbserver running)

The J-Link device name is `APM32E030K8`; SEGGER's database does not carry the
`T6` package suffix.

RTT is read through the GDB server rather than `JLinkRTTLogger` /
`JLinkRTTClient`, because those two cannot locate the RTT control block with the
on-board J-Link (an ARM-OB probe that reports S/N -1).
