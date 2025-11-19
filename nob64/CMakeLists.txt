# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

project(nob64 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(nob64
    nob64.cpp
    # joybus-pio
    joybus-pio/src/joybus.c
    joybus-pio/src/N64Console.cpp
)

pico_generate_pio_header(nob64 ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/joybus.pio)

target_include_directories(nob64
    PRIVATE
        joybus-pio/include
)

pico_set_program_name(nob64 "nob64")
pico_set_program_version(nob64 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(nob64 1)
pico_enable_stdio_usb(nob64 0)

# Add the standard library to the build
target_link_libraries(nob64
        pico_stdlib
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
        hardware_pio
)

# Add the standard include files to the build
target_include_directories(nob64 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(nob64)
