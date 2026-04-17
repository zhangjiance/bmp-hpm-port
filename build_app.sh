#!/bin/bash

# Build script for BMP HPM Port Application
# Usage: ./build_app.sh [uf2|xip] [debug|release]

set -e

# Parse arguments
BUILD_TYPE=${1:-uf2}      # Default: uf2
BUILD_CONFIG=${2:-release} # Default: release

# Set toolchain path
export GNURISCV_TOOLCHAIN_PATH=/home/zhangjiance/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1
export HPM_SDK_BASE=/home/zhangjiance/Code/SDK/hpm_sdk

# Clean old build
echo "Cleaning old build..."
rm -rf build

# Select preset based on arguments
PRESET="hslinklite-${BUILD_TYPE}-${BUILD_CONFIG}"

echo "========================================="
echo "  Building BMP HPM Port Application"
echo "========================================="
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
if [ "${BUILD_TYPE}" == "uf2" ]; then
    ls -lh build/output/*.uf2 2>/dev/null || true
fi
echo "========================================="
