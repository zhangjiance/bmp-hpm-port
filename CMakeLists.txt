# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)


set(APP_NAME "bmp-hpm-port")

set(BOARD "hslinklite")
# set(BOARD "hslinkpro")
set(HPM_BUILD_TYPE "flash_xip")
# set(HPM_BUILD_TYPE "flash_uf2")


if(HPM_BUILD_TYPE STREQUAL "flash_uf2")
set(CUSTOM_GCC_LINKER_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/linkfile/flash_uf2.ld
)
else()
set(CUSTOM_GCC_LINKER_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/linkfile/flash_xip.ld
)
endif()

# set(CONFIG_SEGGER_RTT 1)
set(CONFIG_CHERRYRB 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC_ACM 1)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/board)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherrybmp)

sdk_inc(src)


add_subdirectory(bmp_port)

# sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
# sdk_ses_opt_lib_io_type(RTT)
# sdk_ses_opt_debug_connection(J-Link)
# sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)

sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)
sdk_app_src(main.c)

# generate_ide_projects()

if(HPM_BUILD_TYPE STREQUAL "flash_uf2")
add_custom_command(
        TARGET ${APP_ELF_NAME} POST_BUILD
        COMMAND ${python_exec} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/uf2conv.py -f 0x0A4D5048 -b 0x80020000 -c ${CMAKE_BINARY_DIR}/output/${APP_BIN_NAME} -o ${CMAKE_BINARY_DIR}/output/${APP_NAME}.uf2
        COMMENT "Convert to UF2"
)
endif()
