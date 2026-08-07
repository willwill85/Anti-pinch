# GCC build for the AntiPinch board (APM32E030K8T6, Cortex-M0+).
# The MDK project stays the reference build; this one exists so the firmware
# can be compiled and flashed from macOS/Linux with arm-none-eabi-gcc + J-Link.
#
#   make            build Build/AntiPinch_MDK.hex
#   make flash      build and program over SWD with JLinkExe
#   make gdbserver  start the J-Link GDB server, which also hosts RTT on :19021
#   make rtt        attach to that RTT terminal (run "make gdbserver" first)
#
# The RTT terminal goes through the GDB server on purpose: the on-board J-Link
# (an ARM-OB clone reporting S/N -1) is not found by JLinkRTTLogger /
# JLinkRTTClient, but the GDB server's RTT telnet port works fine.

TARGET  := AntiPinch_MDK
BUILD   := Build
SDK     := ../APM32E030_SDK_V1.0.3

# Homebrew's arm-none-eabi-gcc ships without newlib, so point at a full Arm GNU
# toolchain. Override TOOLCHAIN_BIN if yours lives somewhere else.
TOOLCHAIN_BIN ?= $(HOME)/opt/arm-gnu-toolchain/bin
PREFIX  := $(TOOLCHAIN_BIN)/arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

# SEGGER's device DB lists the part without the T6 package suffix.
DEVICE  := APM32E030K8
JLINK   := JLinkExe

C_SOURCES := \
  Source/main.c \
  Source/apm32e030_int.c \
  Source/rs485_proto.c \
  Source/system_apm32e030.c \
  RTT/SEGGER_RTT.c \
  RTT/SEGGER_RTT_printf.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_adc.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_dma.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_eint.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_gpio.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_misc.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_rcm.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_tmr.c \
  $(SDK)/Libraries/APM32E030_StdPeriphDriver/src/apm32e030_usart.c

ASM_SOURCES := $(SDK)/Libraries/Device/Geehy/APM32E030/Source/gcc/startup_apm32e030.S
LDSCRIPT    := $(SDK)/Libraries/Device/Geehy/APM32E030/Source/gcc/gcc_APM32E03xx8.ld

INCLUDES := \
  -IInclude \
  -IRTT \
  -I$(SDK)/Libraries/APM32E030_StdPeriphDriver/inc \
  -I$(SDK)/Libraries/CMSIS/Include \
  -I$(SDK)/Libraries/Device/Geehy/APM32E030/Include \
  -I$(SDK)/Boards

DEFINES := -DAPM32E030x8 -DBOARD_APM32E030_TINY

CPU     := -mcpu=cortex-m0plus -mthumb
CFLAGS  := $(CPU) $(DEFINES) $(INCLUDES) -Os -g3 -Wall -Wextra \
           -ffunction-sections -fdata-sections -fno-common -MMD -MP
ASFLAGS := $(CPU) -g3 -x assembler-with-cpp
LDFLAGS := $(CPU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

OBJECTS := $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o))) \
           $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.S=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean flash rtt gdbserver

all: $(BUILD)/$(TARGET).hex

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD)/$(TARGET).hex
	printf 'device %s\nsi SWD\nspeed 1000\nr\nhalt\nloadfile %s\nr\ng\nqc\n' \
	  $(DEVICE) $(BUILD)/$(TARGET).hex > $(BUILD)/flash.jlink
	$(JLINK) -NoGui 1 -ExitOnError 1 -CommanderScript $(BUILD)/flash.jlink

gdbserver:
	JLinkGDBServer -device $(DEVICE) -if SWD -speed 1000 -nogui \
	  -rtttelnetport 19021 -noreset

rtt:
	nc 127.0.0.1 19021

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
