# HPM UF2 Bootloader

参考 fcm32_pcan_fd 的 bootloader 实现，为 bmp_hpm_port 项目添加了 UF2 bootloader 支持。

## 功能特性

- **UF2 固件升级**: 支持通过 USB MSC (Mass Storage) 拖放 UF2 文件进行固件更新
- **DFU 支持**: 同时支持 DFU (Device Firmware Update) 协议
- **硬件抽象**: 通过 port 层实现硬件无关的 bootloader 核心
- **安全性**: 包含校验和验证，防止损坏的固件被写入
- **灵活启动**: 支持多种启动条件判断（Boot按钮、RAM magic、应用有效性）

## 目录结构

```
boot/
├── app_main.c                 # Bootloader 主程序
├── bootuf2.c/h                # UF2 核心实现
├── bootuf2_config.h           # UF2 配置
├── boot_log.h                 # 日志输出封装
├── msc_bootuf2_desc.c         # USB MSC + DFU 描述符
├── dfu_webusb.h               # DFU WebUSB HTML 页面
├── CMakeLists.txt             # 构建配置
├── flash_boot.ld              # Bootloader linker script
└── port/                      # 硬件抽象层
    ├── boot_port.h            # 系统控制接口
    ├── boot_flash_port.h      # Flash 操作接口
    ├── boot_port_board.h      # Board 初始化接口
    └── hpm/                   # HPM 平台实现
        ├── boot_flash_port_hpm.c   # HPM Flash 操作（XPI ROM API）
        ├── boot_port_hpm.c         # HPM 系统控制
        └── boot_port_board_hpm.c   # HPM Board 初始化
```

## 内存布局

```
Flash 布局 (1MB total):
┌────────────────────────────────┐ 0x80000000
│  Bootloader (128KB)             │
│  - NOR Config                   │
│  - Boot Header                  │
│  - Bootloader Code              │
├────────────────────────────────┤ 0x80020000
│  Application (896KB)            │
│  - UF2 Signature                │
│  - Application Code             │
├────────────────────────────────┤ 0x80100000
│  Reserved (128KB)               │
│  - Settings / B-slot / etc      │
└────────────────────────────────┘ 0x80120000

Family ID: 0x0A4D5048 (HPMicro)
```

## 构建说明

### 前置条件

1. HPM SDK 环境已配置
2. RISC-V GCC 工具链已安装
3. CMake 3.13+

### 编译 Bootloader

```bash
# 在项目根目录下
mkdir build-boot && cd build-boot

# 配置 CMake (需要专门的 bootloader 配置)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DHPM_BUILD_TYPE=boot \
      -DBOARD=hslinklite \
      ..

# 编译
make

# 生成的文件:
# - hpm_uf2_boot.elf
# - hpm_uf2_boot.bin
# - hpm_uf2_boot.hex
```

### 编译应用程序（用于 UF2）

应用程序需要使用 `flash_uf2.ld` linker script，起始地址为 0x80020000：

```bash
# 设置构建类型
export HPM_BUILD_TYPE=flash_uf2

# 正常编译项目
mkdir build && cd build
cmake .. -DBOARD=hslinklite
make

# 生成 UF2 文件
# （通常在主 CMakeLists.txt 中已配置自动生成）
```

## 使用说明

### 1. 烧写 Bootloader

首次使用需要烧写 bootloader：

```bash
# 使用 OpenOCD 或其他工具烧写
openocd -f openocd.cfg -c "program hpm_uf2_boot.elf verify reset exit"

# 或使用 HPMicro 工具
```

### 2. 进入 Bootloader 模式

有三种方式进入 bootloader：

1. **Boot 按钮**: 按住 Boot 按钮上电或复位
2. **软件触发**: 应用程序写入 magic value 到 `0xF0400000` 后复位
3. **无有效应用**: Flash 中没有有效应用程序时自动进入

### 3. 更新固件

#### 方式 1: UF2 拖放（推荐）

1. 进入 bootloader 模式
2. PC 会识别出一个名为 "CHERRYUF2" 的 USB 存储设备
3. 将 `.uf2` 文件拖放到该设备
4. 等待传输完成（LED 可能闪烁）
5. 设备自动重启并运行新固件

#### 方式 2: DFU 工具

```bash
# 使用 dfu-util
dfu-util -d 34BF:0003 -a 0 -D firmware.bin

# 或使用 WebUSB DFU 工具
# 打开 USB 设备中的 DFU.HTM 文件
```

## Port 层接口说明

### boot_flash_port.h - Flash 操作接口

```c
uint32_t boot_flash_port_get_app_start(void);        // 获取应用起始地址
uint32_t boot_flash_port_get_app_size(void);         // 获取应用大小
uint32_t boot_flash_port_get_page_size(void);        // 获取 Flash 页大小
int boot_flash_port_write(uint32_t addr, const uint8_t *data, size_t size);
int boot_flash_port_erase_app(void);                 // 擦除应用区域
bool boot_flash_port_check_app_valid(void);          // 检查应用有效性
```

### boot_port.h - 系统控制接口

```c
void boot_port_jump_to_app(void);                    // 跳转到应用
void boot_port_system_reset(void);                   // 系统复位
bool boot_port_check_bootloader_request(void);       // 检查是否需要进入 bootloader
void boot_port_delay_ms(uint32_t ms);                // 延时
```

### boot_port_board.h - Board 初始化接口

```c
void boot_port_board_init(void);                     // Board 初始化
void boot_port_board_deinit(void);                   // Board 反初始化
bool boot_port_board_read_bootpin(void);             // 读取 Boot 按钮状态
```

## 移植到其他平台

如需移植到其他 HPM 芯片或其他 RISC-V 平台：

1. 在 `port/` 下创建新的平台目录（如 `port/hpm6750/`）
2. 实现三个 port 文件：
   - `boot_flash_port_xxx.c` - 实现 Flash 操作
   - `boot_port_xxx.c` - 实现系统控制
   - `boot_port_board_xxx.c` - 实现 Board 初始化
3. 修改 `bootuf2_config.h` 中的配置（Family ID、Flash 大小等）
4. 修改 `flash_boot.ld` 以适配内存布局
5. 更新 CMakeLists.txt 以包含新的源文件

## 配置选项

在 `bootuf2_config.h` 中可配置：

```c
#define CONFIG_BOOTUF2_FAMILYID      0x0A4D5048  // HPM family ID
#define CONFIG_BOOTUF2_FLASHMAX      0xE0000     // 应用区域大小 (896KB)
#define CONFIG_BOOTUF2_CACHE_SIZE    2048        // 写缓存大小
#define CONFIG_BOOTUF2_SECTOR_SIZE   512         // FAT16 扇区大小
```

## 调试

Bootloader 通过 UART 输出调试信息：

```
========================================
  HPM UF2 Bootloader
  Version: 1.0.0
  Build: Apr 15 2026 10:30:00
========================================
[BOOT] No bootloader request, jumping to application...
```

波特率: 115200 (默认 BOARD_CONSOLE_UART_BAUDRATE)

## 已知问题和限制

1. **Flash 磨损**: 频繁更新会导致 Flash 磨损，建议生产环境合理使用
2. **更新时间**: 896KB 应用需要约 30-60 秒完成更新
3. **兼容性**: UF2 文件必须包含正确的 Family ID (0x0A4D5048)

## 参考资料

- [UF2 File Format](https://github.com/microsoft/uf2)
- [HPM SDK Documentation](https://github.com/hpmicro/hpm_sdk)
- [USB DFU Specification](https://www.usb.org/document-library/device-firmware-upgrade-11-new-version-12)
- fcm32_pcan_fd bootloader 实现

## 许可证

遵循 HPM SDK 的 BSD-3-Clause 许可证。

UF2 bootloader 核心代码基于 sakumisu/Cherryuf2，遵循 Apache-2.0 许可证。
