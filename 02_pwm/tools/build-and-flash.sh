#!/usr/bin/env zsh
set -ue
PORT=$1

mkdir -p build

CFLAGS="-W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Wformat-truncation \
    -fno-common -Wconversion -g3 -O0 -ffunction-sections -fdata-sections -I. \
    -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16"

LDFLAGS="-Tf103.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc \
    -Wl,--gc-sections -Wl,-Map=build/firmware.elf.map"

arm-none-eabi-gcc main.c $(echo ${CFLAGS}) $(echo ${LDFLAGS}) -o build/firmware.elf
arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin
stm32flash -w build/firmware.bin -v -g 0x0 ${PORT}
