cmake_minimum_required(VERSION 3.13)

set(APP_NAME hpm-uf2-boot)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../board)

# RISC-V architecture with CSR and fence extensions
set(RV_ARCH "rv32imac_zicsr_zifencei")
set(RV_ABI "ilp32")

# Bootloader linker script
set(CUSTOM_GCC_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/flash_boot.ld)
set(HEAP_SIZE 0x800)
set(STACK_SIZE 0x2000)

# Enable CherryUSB (DFU-only baseline)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_DFU 1)
set(CONFIG_SEGGER_RTT 1)
# NOTE: HPM SDK cherryusb CMake currently does not gate usbd_core on DFU flag,
# so keep one standard device class enabled to pull in USB device core/driver.
set(CONFIG_USB_DEVICE_MSC 1)

# Find HPM SDK
find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(hpm_uf2_boot)

# Generate dfu_webusb.h from HTML - must be before source compilation
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dfu_webusb.h
    COMMAND ${CMAKE_COMMAND} -E echo "Generating dfu_webusb.h..."
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/file_to_header.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/webdfu_ultra.html
            ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/file_to_header.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    COMMENT "Generating dfu_webusb.h from webdfu_ultra.html"
)

add_custom_target(generate_webdfu_header 
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dfu_webusb.h
)

# Include CherryUSB DFU class manually (HPM SDK doesn't include it by default)
sdk_inc($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/dfu)

# Include paths
sdk_inc(.)
sdk_inc(port)

# Source files - Full MSC + DFU bootloader
sdk_app_src(app_main.c)
sdk_app_src(bootuf2.c)
sdk_app_src(msc_bootuf2_desc.c)
sdk_app_src(port/hpm/boot_flash_port_hpm.c)
sdk_app_src(port/hpm/boot_port_hpm.c)
sdk_app_src(port/hpm/boot_port_board_hpm.c)

# Add DFU class source (not included by HPM SDK by default)
sdk_app_src($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/dfu/usbd_dfu.c)

# Compile definitions
sdk_compile_definitions(-DCONFIG_USB_HS=1)  # Use High Speed for maximum performance
sdk_compile_definitions(-DBOARD_UF2_SIGNATURE=0x0A4D5048)
sdk_compile_definitions(-DUSBD_DFU_XFER_SIZE=4096)  # 4KB - testing Windows driver limit
sdk_compile_definitions(-DUSBD_DFU_APP_DEFAULT_ADD=0x80020000)
sdk_compile_definitions(-DFLASH_ERASE_TIME=15000)  # 15 seconds for full 896KB erase
sdk_compile_definitions(-DFLASH_PROGRAM_TIME=120)  # 120ms per 4KB write
sdk_compile_definitions(-DCONFIG_USB_PRINTF=printf)
sdk_compile_definitions(-DCONFIG_USB_DBG_LEVEL=2)
sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
sdk_compile_definitions(-DGDB_PACKET_BUFFER_SIZE=16384)

# Disable BOOT_PRINTF and printf in Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    sdk_compile_definitions(-DNDEBUG)
    message(STATUS "Release build: BOOT_PRINTF disabled")
endif()

# Generate IDE projects and ensure HTML header is generated before compilation
generate_ide_projects()
add_dependencies(app generate_webdfu_header)
