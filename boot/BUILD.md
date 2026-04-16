# UF2 Bootloader 编译说明

## 使用 CMake Presets (推荐)

项目已配置CMake Presets，可以轻松切换不同的编译配置。

### 查看可用的预设配置

```bash
cd boot
cmake --list-presets
```

可用配置：
- `hslinklite-boot` - HSLink Lite Release版本 (推荐)
- `hslinklite-boot-debug` - HSLink Lite Debug版本 (带调试符号)
- `hslinkpro-boot` - HSLink Pro Release版本
- `hslinkpro-boot-debug` - HSLink Pro Debug版本

### 方法1: 使用CMake命令

```bash
cd boot

# 配置 (使用preset)
cmake --preset hslinklite-boot

# 编译
cmake --build build

# 清理重新编译
rm -rf build
cmake --preset hslinklite-boot
cmake --build build
```

### 方法2: 使用构建脚本

```bash
cd boot

# 默认编译 (hslinklite-boot)
./build_bootloader.sh

# 指定preset
./build_bootloader.sh hslinklite-boot-debug

# 清理重新编译
./build_bootloader.sh hslinklite-boot --clean

# 查看可用preset
./build_bootloader.sh --list
```

## 编译输出

成功编译后，输出文件在 `boot/build/output/`:
- `hpm-uf2-boot.bin` - 二进制文件 (用于烧录)
- `hpm-uf2-boot.elf` - ELF文件 (带符号信息)
- `hpm-uf2-boot.map` - 内存映射文件
- `hpm-uf2-boot.asm` - 反汇编文件

## 配置说明

预设配置定义在 `boot/CMakePresets.json`:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| 工具链路径 | `/tmp/riscv-hpm-toolchain` | RISC-V GCC工具链 |
| HPM SDK | `$env{HOME}/Code/SDK/hpm_sdk` | HPM SDK路径 |
| 编译器 | `riscv32-unknown-elf-gcc` | RISC-V 32位编译器 |
| 生成器 | `Ninja` | 快速增量编译 |
| 优化级别 | `-O3` (Release) / `-g` (Debug) | 优化/调试 |

## 手动配置 (不推荐)

如果需要手动配置，可以使用以下命令：

```bash
cd boot
rm -rf build && mkdir build && cd build

export GNURISCV_TOOLCHAIN_PATH=/tmp/riscv-hpm-toolchain
export HPM_SDK_BASE=$HOME/Code/SDK/hpm_sdk

cmake .. -DBOARD=hslinklite -GNinja
ninja
```

## 烧录

使用OpenOCD烧录到Flash起始地址 0x80000000:

```bash
openocd -f your_config.cfg \
  -c "program boot/build/output/hpm-uf2-boot.elf verify reset exit"
```

或使用其他调试器工具烧录 `.bin` 文件到 0x80000000。

## 内存布局

- **Bootloader**: 0x80000000 - 0x80020000 (128KB)
- **Application**: 0x80020000 - 0x80100000 (896KB)
- **Reserved**: 0x80100000 - 0x80120000 (128KB)

## 功能特性

- ✅ USB MSC - UF2拖放升级
- ✅ USB DFU - 标准DFU升级
- ✅ WebUSB - 浏览器网页升级
- ✅ 双启动 - Slot A/B切换
- ✅ 按钮强制进入bootloader

## 故障排查

### 工具链未找到

确保软链接存在：
```bash
ls -la /tmp/riscv-hpm-toolchain/bin/riscv32-unknown-elf-gcc
```

如果不存在，创建软链接：
```bash
ln -s ~/Code/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin /tmp/riscv-hpm-toolchain/bin
```

### HPM SDK未找到

设置环境变量：
```bash
export HPM_SDK_BASE=$HOME/Code/SDK/hpm_sdk
```

或修改 `CMakePresets.json` 中的路径。
