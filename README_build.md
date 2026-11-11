# bmp-hpm-port Build Guide

This repository ports Black Magic Debug to HPMicro RISC-V MCUs (HPM5301) and is built with the HPM SDK and a RISC-V cross-compilation toolchain. This document explains how to set up the environment and build the project with CMake presets.

> 中文版见 [README_build_zh.md](./README_build_zh.md)。Project overview: [README.md](./README.md) (English) / [README_zh.md](./README_zh.md) (中文).

---

## 1. Prerequisites (required)

Before building, you **must** have:

1. The HPM SDK source tree, version **1.13**;
2. A RISC-V cross-compilation toolchain (`riscv-none-elf`), with the environment variables below set correctly.

> The toolchain version is up to you — any xpack `riscv-none-elf-gcc` release works. The HPM SDK should be **1.13**. Both the SDK and the toolchain are indispensable. `CMakePresets.json` does not hard-code any compiler or SDK path; all paths are read from environment variables.

### 1.1 Get the HPM SDK

```bash
git clone https://github.com/hpmicro/hpm_sdk.git ~/Code/SDK/hpm_sdk
# checkout the 1.13 release/tag
```

### 1.2 Get the RISC-V toolchain

`CMakePresets.json` sets `CUSTOM_TARGET_TRIPLET=riscv-none-elf`. Download a matching xpack release, for example:

```bash
# Example: xpack riscv-none-elf-gcc 15.2.0-1
# Extract to ~/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1
```

The toolchain directory must contain `bin/riscv-none-elf-gcc` and the other executables.

Optional (for VS Code): the `C/C++`, `CMake`, `CMake Tools` and `HPM Pinmux Tool` extensions.

---

## 2. Environment Variables

Add the following to `~/.bashrc` (or the config file for your shell), then `source ~/.bashrc` to apply:

```bash
# Add the toolchain bin to PATH (replace with your actual extracted directory)
export PATH="$HOME/.local/bin:$HOME/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"

# Point to your local HPM SDK root
export HPM_SDK_BASE="$HOME/Code/SDK/hpm_sdk"

# Point to the toolchain root, used by HPM SDK to locate the compiler
export GNURISCV_TOOLCHAIN_PATH="$HOME/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1"
```

Meaning of each variable:

| Variable | Purpose |
| --- | --- |
| `PATH` | Makes `cmake` / `ninja` and the `riscv-none-elf-*` compilers directly callable. |
| `HPM_SDK_BASE` | `CMakeLists.txt` locates the SDK via `find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})`. |
| `GNURISCV_TOOLCHAIN_PATH` | Used internally by HPM SDK to find the RISC-V toolchain. |

> You can also `source <hpm_sdk>/env.sh` to set `HPM_SDK_BASE` and `OPENOCD_SCRIPTS` automatically, but the toolchain-related variables still need to be set manually.

Verify the environment:

```bash
echo $HPM_SDK_BASE
echo $GNURISCV_TOOLCHAIN_PATH
which riscv-none-elf-gcc cmake ninja
```

---

## 3. Build with CMake Presets (recommended)

```bash
# List all available presets
cmake --list-presets

# Configure and build (example: HSLink Lite XIP Release)
cmake --preset hslinklite-xip-release
cmake --build --preset hslinklite-xip-release
```

Or build directly with `ninja`:

```bash
ninja -C build -j8
```

### 3.1 Supported presets

Preset names follow `<board>-<flash type>-<build type>`:

| Board | XIP Release | XIP Debug | UF2 Release | UF2 Debug |
| --- | --- | --- | --- | --- |
| HSLink Lite | `hslinklite-xip-release` | `hslinklite-xip-debug` | `hslinklite-uf2-release` | `hslinklite-uf2-debug` |
| HSLink Pro | `hslinkpro-xip-release` | `hslinkpro-xip-debug` | `hslinkpro-uf2-release` | `hslinkpro-uf2-debug` |

On this branch the XIP presets use `HPM_BUILD_TYPE=flash_xip` and the UF2 presets use `HPM_BUILD_TYPE=flash_uf2`. For UF2 builds, `scripts/uf2conv.py` converts the binary into a `.uf2` image after linking. See `CMakePresets.json` for the exact cache variables of each preset.

---

## 4. Build Artifacts

All presets on this branch share the same `binaryDir` (`${sourceDir}/build`), so the output is located at:

```
build/output/bmp-hpm-port.elf
build/output/bmp-hpm-port.bin
build/output/bmp-hpm-port.uf2   # flash_uf2 presets only
```

- `.elf`: executable image, for debugging / flashing with OpenOCD;
- `.bin`: raw binary, convenient for flashing tools;
- `.uf2`: UF2 image for drag-and-drop flashing via the USB mass-storage bootloader.

---

## 5. Switching Presets

Because every preset writes into the same `build/` directory, always clean it before switching to a different board, flash type or build type:

```bash
rm -rf build
cmake --preset hslinkpro-uf2-release
cmake --build --preset hslinkpro-uf2-release
```

This avoids stale `BOARD` / `HPM_BUILD_TYPE` / `CMAKE_BUILD_TYPE` cache values from the previous preset.

---

## 6. FAQ

- **`HPM SDK not found` / `find_package(hpm-sdk) failed`**
  Check that `HPM_SDK_BASE` is exported and points to the root of HPM SDK 1.13.

- **`riscv-none-elf-gcc` not found**
  Confirm the toolchain `bin` directory is on `PATH` and `GNURISCV_TOOLCHAIN_PATH` is set correctly.

- **`BOARD is not set`**
  No preset was selected, or `-DBOARD=<board>` was not specified.

- **CMake keeps using an old compiler or SDK path**
  Delete `build/` (or just `build/CMakeCache.txt`) and reconfigure; cached variables are sticky.
