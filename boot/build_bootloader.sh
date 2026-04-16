#!/bin/bash
# UF2 Bootloader Build Script
# Uses CMakePresets.json for configuration

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default preset
PRESET="${1:-hslinklite-boot}"

echo "========================================="
echo "Building UF2 Bootloader"
echo "========================================="
echo "Preset: $PRESET"
echo ""

# Show available presets
if [ "$1" = "--list" ] || [ "$1" = "-l" ]; then
    echo "Available presets:"
    cmake --list-presets
    exit 0
fi

# Clean build (optional)
if [ "$2" = "--clean" ] || [ "$2" = "-c" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Configure with preset
echo "Configuring with preset: $PRESET"
cmake --preset "$PRESET"

echo ""
echo "Building..."
cmake --build build

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo "Output files:"
ls -lh build/output/*.{elf,bin,map} 2>/dev/null || true
echo ""
echo "Memory usage:"
/tmp/riscv-hpm-toolchain/bin/riscv32-unknown-elf-size build/output/hpm-uf2-boot.elf

echo ""
echo "To rebuild with different preset:"
echo "  $0 hslinklite-boot          # Release build"
echo "  $0 hslinklite-boot-debug    # Debug build with symbols"
echo "  $0 hslinkpro-boot           # HSLink Pro variant"
echo ""
echo "To clean and rebuild:"
echo "  $0 $PRESET --clean"

