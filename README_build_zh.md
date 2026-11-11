# bmp-hpm-port 编译指导

本仓库将 Black Magic Debug 移植到 HPMicro RISC-V MCU（HPM5301），基于 HPM SDK 与 RISC-V 交叉编译工具链构建。本文档说明如何配置环境并使用 CMake preset 完成编译。

> English version: [README_build.md](./README_build.md). 工程说明：[README.md](./README.md)（英文）/ [README_zh.md](./README_zh.md)（中文）。

---

## 1. 前置准备（必要步骤）

在编译之前，**必须**具备以下两项：

1. HPM SDK 源码，版本 **1.13**；
2. RISC-V 交叉编译工具链（`riscv-none-elf`），并正确设置相关环境变量。

> 工具链的**具体版本可以自行选择**，任意 xpack `riscv-none-elf-gcc` 发行版均可；HPM SDK 建议使用 **1.13**。二者缺一不可。`CMakePresets.json` 中没有硬编码任何编译器或 SDK 路径，全部从环境变量读取。

### 1.1 获取 HPM SDK

```bash
git clone https://github.com/hpmicro/hpm_sdk.git ~/Code/SDK/hpm_sdk
# 切换到 1.13 对应的版本/标签
```

### 1.2 获取 RISC-V 工具链

`CMakePresets.json` 中指定了 `CUSTOM_TARGET_TRIPLET=riscv-none-elf`，可下载对应的 xpack 版本，例如：

```bash
# 示例：xpack riscv-none-elf-gcc 15.2.0-1
# 解压到 ~/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1
```

工具链目录结构应至少包含 `bin/riscv-none-elf-gcc` 等可执行文件。

可选（使用 VSCode 开发时）：`C/C++`、`CMake`、`CMake Tools`、`HPM Pinmux Tool` 插件。

---

## 2. 环境变量设置

将以下内容加入 `~/.bashrc`（或对应 shell 的配置文件），然后 `source ~/.bashrc` 使其生效：

```bash
# 将工具链 bin 加入 PATH（请替换为你实际解压的目录）
export PATH="$HOME/.local/bin:$HOME/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"

# 指向你本地的 HPM SDK 根目录
export HPM_SDK_BASE="$HOME/Code/SDK/hpm_sdk"

# 指向工具链根目录，HPM SDK 在查找编译器时会用到
export GNURISCV_TOOLCHAIN_PATH="$HOME/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1"
```

各变量含义：

| 变量 | 作用 |
| --- | --- |
| `PATH` | 让 `cmake` / `ninja` 以及 `riscv-none-elf-*` 编译器可被直接调用。 |
| `HPM_SDK_BASE` | 工程的 `CMakeLists.txt` 通过 `find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})` 定位 SDK。 |
| `GNURISCV_TOOLCHAIN_PATH` | HPM SDK 内部据此寻找 RISC-V 工具链。 |

> 也可 `source <hpm_sdk>/env.sh` 自动设置 `HPM_SDK_BASE` 与 `OPENOCD_SCRIPTS`，但工具链相关变量仍需手动设置。

验证环境：

```bash
echo $HPM_SDK_BASE
echo $GNURISCV_TOOLCHAIN_PATH
which riscv-none-elf-gcc cmake ninja
```

---

## 3. 使用 CMake Preset 编译（推荐）

```bash
# 查看全部 preset
cmake --list-presets

# 配置并构建（以 HSLink Lite XIP Release 为例）
cmake --preset hslinklite-xip-release
cmake --build --preset hslinklite-xip-release
```

或使用 `ninja` 直接指定 jobs：

```bash
ninja -C build/hslinklite-xip-release -j8
```

### 3.1 支持的 Preset

命名规则为 `<board>-<flash类型>-<build类型>`：

| Board | XIP Release | XIP Debug | DFU Release | DFU Debug |
| --- | --- | --- | --- | --- |
| HSLink Lite | `hslinklite-xip-release` | `hslinklite-xip-debug` | `hslinklite-dfu-release` | `hslinklite-dfu-debug` |
| HSLink Pro | `hslinkpro-xip-release` | `hslinkpro-xip-debug` | `hslinkpro-dfu-release` | `hslinkpro-dfu-debug` |

本分支的基础 preset 使用 `HPM_BUILD_TYPE=flash_dfu`，并通过 `sdk_linker_global_symbols("_dfu_bl_length=0x20000")` 为 DFU bootloader 预留 128KB。各 preset 的具体缓存变量见 `CMakePresets.json`。

---

## 4. 编译产物

每个 preset 有独立的 `binaryDir`，产物位于：

```
build/<preset>/output/bmp-hpm-port.elf
build/<preset>/output/bmp-hpm-port.bin
```

- `.elf`：可执行镜像，可用于调试 / OpenOCD 烧录；
- `.bin`：裸二进制，便于通过烧录工具写入 Flash。

---

## 5. 不使用 Preset 的手动编译

如果希望手动指定参数而不使用 preset，可显式传入 `BOARD` 及工具链相关变量：

```bash
cmake -B build/manual \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBOARD=hslinklite \
  -DHPM_BUILD_TYPE=flash_dfu \
  -DRV_ARCH=rv32imac_zicsr_zifencei \
  -DCUSTOM_TARGET_TRIPLET=riscv-none-elf \
  -DCMAKE_TOOLCHAIN_FILE=$HPM_SDK_BASE/cmake/toolchain/riscv_none_elf_gcc.cmake

cmake --build build/manual -j8
```

注意：手动编译时同样需要第 2 节中的 `HPM_SDK_BASE` 与 `GNURISCV_TOOLCHAIN_PATH` 环境变量。

---

## 6. 常见问题

- **`HPM SDK not found` / `find_package(hpm-sdk) failed`**
  检查 `HPM_SDK_BASE` 是否已导出且指向 HPM SDK 1.13 的根目录。

- **找不到 `riscv-none-elf-gcc`**
  确认工具链 `bin` 目录已加入 `PATH`，且 `GNURISCV_TOOLCHAIN_PATH` 设置正确。

- **提示 `BOARD is not set`**
  未选用任何 preset，或未通过 `-DBOARD=<board>` 指定目标板。

- **CMake 仍在使用旧的编译器 / SDK 路径**
  删除对应 preset 的构建目录（或仅删除其中的 `CMakeCache.txt`）后重新配置；缓存变量不会自动更新。
