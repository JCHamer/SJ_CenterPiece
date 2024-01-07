cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(SJ_CenterPiece C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

add_executable(pio_SJ_CenterPiece)

# by default the header is generated into the build dir
pico_generate_pio_header(pio_SJ_CenterPiece ${CMAKE_CURRENT_LIST_DIR}/SJ_CenterPiece.pio)
# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
#pico_generate_pio_header(pio_SJ_CenterPiece ${CMAKE_CURRENT_LIST_DIR}/SJ_CenterPiece.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(pio_SJ_CenterPiece PRIVATE SJ_CenterPiece.c)

if (TARGET tinyusb_device)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(pio_SJ_CenterPiece 1)
    pico_enable_stdio_uart(pio_SJ_CenterPiece 0)
elseif(PICO_ON_DEVICE)
	message(WARNING "not building SJ_CenterPiece because TinyUSB submodule is not initialized in the SDK")
endif()

# pull in common dependencies
target_link_libraries(pio_SJ_CenterPiece PRIVATE pico_stdlib hardware_pio hardware_sleep)

# create map/bin/hex file etc.
pico_add_extra_outputs(pio_SJ_CenterPiece)

# add url via pico_set_program_url
#example_auto_set_url(pio_SJ_CenterPiece)

