cmake_minimum_required(VERSION 3.13)

set(APP_NAME hpm-dfu-boot)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../boards)

# RISC-V architecture with CSR and fence extensions
set(RV_ARCH "rv32imac_zicsr_zifencei")
set(RV_ABI "ilp32")

# Bootloader linker script
set(CUSTOM_GCC_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linkfile/flash_boot.ld)
set(HEAP_SIZE 0x0)
set(STACK_SIZE 0x2000)

# Prevent SDK from forcing -u _printf_float/-u _scanf_float (saves ~31KB)
set(HPM_SDK_LD_NO_NANO_SPECS 1)

# Enable CherryUSB (DFU only)
# CONFIG_CHERRYUSB_DEVICE must be set explicitly because the SDK's
# cherryusb CMakeLists.txt does not include CONFIG_USB_DEVICE_DFU in
# the condition that triggers device core compilation.
set(CONFIG_CHERRYUSB 1)
set(CONFIG_CHERRYUSB_DEVICE 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_DFU 1)

# Find HPM SDK
find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(hpm_dfu_boot)

# Use nano libc without printf_float/scanf_float (saves ~31KB)
sdk_ld_options("--specs=nano.specs")

# Include CherryUSB DFU class manually (HPM SDK doesn't include it by default)
sdk_inc($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/dfu)

# Include paths
sdk_inc(.)
sdk_inc(port)

# CherryUSB config (must be before cherryusb includes)
sdk_compile_definitions(-D__INCLUDE_CHERRYUSB_CONFIG_H__)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR})

# Source files - DFU-only bootloader
sdk_app_src(app_main.c)
sdk_app_src(dfu_desc.c)
sdk_app_src(port/hpm/boot_flash_port_hpm.c)
sdk_app_src(port/hpm/boot_port_hpm.c)
sdk_app_src(port/hpm/boot_port_board_hpm.c)

# CherryUSB DFU class source
sdk_app_src($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/dfu/usbd_dfu.c)

# Compile definitions
sdk_compile_definitions(-DUSBD_DFU_XFER_SIZE=4096)

# Disable BOOT_PRINTF and UART console in Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    sdk_compile_definitions(-DNDEBUG)
    sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
    message(STATUS "Release build: BOOT_PRINTF + console disabled")
endif()
