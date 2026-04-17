#!/bin/bash

# Build script for BMP HPM Port Bootloader
# Usage: ./build_boot.sh [lite|pro] [debug|release]

set -e

# Parse arguments
BOARD=${1:-lite}           # Default: lite
BUILD_CONFIG=${2:-release} # Default: release

# Set toolchain path
export GNURISCV_TOOLCHAIN_PATH=/home/zhangjiance/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1
export HPM_SDK_BASE=/home/zhangjiance/Code/SDK/hpm_sdk

# Enter boot directory
cd boot

# Clean old build
echo "Cleaning old build..."
rm -rf build

# Select preset based on arguments
if [ "${BOARD}" == "lite" ]; then
    BOARD_NAME="hslinklite"
elif [ "${BOARD}" == "pro" ]; then
    BOARD_NAME="hslinkpro"
else
    echo "Error: Invalid board '${BOARD}'. Use 'lite' or 'pro'"
    exit 1
fi

if [ "${BUILD_CONFIG}" == "debug" ]; then
    PRESET="${BOARD_NAME}-boot-debug"
elif [ "${BUILD_CONFIG}" == "release" ]; then
    PRESET="${BOARD_NAME}-boot"
else
    echo "Error: Invalid config '${BUILD_CONFIG}'. Use 'debug' or 'release'"
    exit 1
fi

echo "========================================="
echo "  Building UF2 Bootloader"
echo "========================================="
echo "Board: ${BOARD_NAME}"
echo "Config: ${BUILD_CONFIG}"
echo "Preset: ${PRESET}"
echo "Toolchain: ${GNURISCV_TOOLCHAIN_PATH}"
echo "SDK: ${HPM_SDK_BASE}"
echo "========================================="

# Configure
echo "Configuring..."
cmake --preset ${PRESET}

# Build
echo "Building..."
ninja -C build

echo ""
echo "========================================="
echo "  Build Complete!"
echo "========================================="
echo "Output files:"
ls -lh build/output/*.elf build/output/*.bin build/output/*.map 2>/dev/null || true
echo "========================================="
echo ""
echo "Flash command:"
echo "  openocd -f board/${BOARD_NAME}/${BOARD_NAME}.cfg \\"
echo "    -c \"program build/output/hpm-uf2-boot.elf verify reset exit\""
echo "========================================="
