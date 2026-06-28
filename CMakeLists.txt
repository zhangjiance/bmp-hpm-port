# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(RV_ARCH "rv32imac_zicsr_zifencei")

set(APP_NAME "bmp-hpm-port")


set(CUSTOM_GCC_LINKER_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/linkfile/flash_dfu.ld
)

set(CONFIG_SEGGER_RTT 1)
set(CONFIG_CHERRYRB 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC_ACM 1)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boards)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherrybmp)

sdk_inc(src)


add_subdirectory(bmp_port)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
sdk_compile_definitions(-DGDB_PACKET_BUFFER_SIZE=16384)

sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)
sdk_app_src(main.c)

# generate_ide_projects()
