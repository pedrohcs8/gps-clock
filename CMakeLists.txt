# Set Minimum Cmake Version
cmake_minimum_required(VERSION 3.12)

# Include Build Functions
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(gps-clock C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.c
    displays.c
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Make UF2
pico_add_extra_outputs(${PROJECT_NAME})

# Link to STDLIB
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_gpio
    hardware_uart
    pico_multicore
)

add_compile_definitions(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=5000)
