cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(usb_to_uart C CXX ASM)


if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

if (TARGET tinyusb_device)
    add_executable(usb_to_uart
    usb_to_uart.c
            )

    # pull in common dependencies
    target_link_libraries(usb_to_uart pico_stdlib hardware_uart)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(usb_to_uart 1)
    pico_enable_stdio_uart(usb_to_uart 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(usb_to_uart)

    # add url via pico_set_program_url
    # example_auto_set_url(usb_to_uart)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building usb_to_uart because TinyUSB submodule is not initialized in the SDK")
endif()
