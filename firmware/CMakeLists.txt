# DO NOT MODIFY
cmake_minimum_required(VERSION 3.15)

# Project configuration
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

# Skip compiler test (cross-compiling)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(firmware C ASM)

# MCU-specific flags
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -Wall -Wextra -fdata-sections -ffunction-sections -O2 -g")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${CMAKE_SOURCE_DIR}/STM32F411_FLASH.ld -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map -specs=nano.specs -specs=nosys.specs -Wl,--wrap=malloc")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/freertos/include
    ${CMAKE_SOURCE_DIR}/freertos/portable/GCC/ARM_CM4F
)

# FreeRTOS source files
set(FREERTOS_SOURCES
    ${CMAKE_SOURCE_DIR}/freertos/tasks.c
    ${CMAKE_SOURCE_DIR}/freertos/queue.c
    ${CMAKE_SOURCE_DIR}/freertos/list.c
    ${CMAKE_SOURCE_DIR}/freertos/timers.c
    ${CMAKE_SOURCE_DIR}/freertos/portable/GCC/ARM_CM4F/port.c
    ${CMAKE_SOURCE_DIR}/freertos/portable/MemMang/heap_none.c
)

# Firmware source files (exclude malloc_wrapper.c as it's handled separately)
file(GLOB FIRMWARE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
)
list(REMOVE_ITEM FIRMWARE_SOURCES ${CMAKE_SOURCE_DIR}/src/malloc_wrapper.c)

# Startup file
set(STARTUP_FILE ${CMAKE_SOURCE_DIR}/startup_stm32f411xe.s)

# Create executable
add_executable(firmware.elf
    ${STARTUP_FILE}
    ${FREERTOS_SOURCES}
    ${FIRMWARE_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/malloc_wrapper.c
)

# Post-build: generate .bin and .hex files
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex firmware.elf firmware.hex
    COMMAND ${CMAKE_SIZE} firmware.elf
    COMMENT "Generating binary and hex files, showing size"
)

# Print build info
message(STATUS "Target: STM32F411RE")
message(STATUS "Toolchain: ${CMAKE_C_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
