# bmp-hpm-port

本仓库是基于blackmagic的hpm平台移植，目前适配：

- hslinklite(hpm5301)
- hslinkpro(hpm5301)

## 工程说明

### 依赖说明

目前工程使用vscode进行开发，建议安装以下插件：

- `C/C++`
- `CMake`
- `CMake Tools`
- `HPM Pinmux Tool`

工程依赖hpm_sdk，建议使用`1.8`版本。
使用时需要将`hpm_sdk`路径及工具链添加到环境变量中,也可在工程settings.json中配置。

- `HPM_SDK_BASE`：hpm_sdk的安装路径
  - 示例：`D:/sdk_env_v1.8.0/hpm_sdk`
- `GNURISCV_TOOLCHAIN_PATH`：riscv toolchain的安装路径,无特殊需求，可使用sdk_env_v1.8.0/toolchains中的版本
  - 示例：`D:/sdk_env_v1.8.0/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win`

## 效果展示

使用hpm做移植的好处是可以通过fgpio和spi外设，使得swd/jtag时钟速度更快，从而提高调试效率。

- 目前适配了fgpio模式，spi模式正在移植中。

![alt text](image/fgpio-swd-speed.png)

## bmp命令使用说明

- `xxx-gdb yyy.elf`：启动gdb并加载elf文件
- `target extended-remote //./COMx`：连接GDBServer，其中COMx为串口号
- `monitor jtag_scan`:通过jtag扫描设备
- `monitor swd_scan`：通过swd扫描设备
- `monitor auto_scan`：自动扫描
- `attach 1`：连接到设备号为1的设备
- `load`：加载elf文件到设备中

### 示例

```bash
(gdb) target extended-remote //./COM8
Remote debugging using //./COM8
(gdb) monitor help
(gdb) monitor jtag_scan
Target voltage: Unknown
Available Targets:
No. Att Driver
 1      STM32F40x M4
(gdb) attach 1
(gdb) load
Loading section .isr_vector, size 0x188 lma 0x8000000
Loading section .text, size 0x4898 lma 0x8000190
Loading section .rodata, size 0x60 lma 0x8004a28
Loading section .ARM, size 0x8 lma 0x8004a88
Loading section .init_array, size 0x4 lma 0x8004a90
Loading section .fini_array, size 0x4 lma 0x8004a94
Loading section .data, size 0x10 lma 0x8004a98
Start address 0x0800468c, load size 19104
Transfer rate: 32 KB/sec, 764 bytes/write.
(gdb)
```

详细使用方法请参考：https://black-magic.org/index.html

## Cortex-Debug配置

对于不习惯命令行操作gdb的用户，可以使用Cortex-Debug插件。Cortex-Debug对bmp提供了良好的支持。

swd配置文件参考:

```json
{
    "name": "bmp-launch-swd",
    "cwd": "${workspaceFolder}",
    "executable": "./build/LED_TEST_F407ZE.elf",
    "request": "launch",
    "type": "cortex-debug",
    "runToEntryPoint": "main",
    "servertype": "bmp",
    "device": "STM32F407ZE",
    "BMPGDBSerialPort": "//./COM8",
    "interface": "swd",
    "gdbPath": "C:/Develop_Software/gdb-multiarch-14.1/bin/gdb-multiarch.exe",
}
```

jtag配置文件参考:

```json
{
    "name": "bmp-launch-swd",
    "cwd": "${workspaceFolder}",
    "executable": "./build/LED_TEST_F407ZE.elf",
    "request": "launch",
    "type": "cortex-debug",
    "runToEntryPoint": "main",
    "servertype": "bmp",
    "device": "STM32F407ZE",
    "BMPGDBSerialPort": "//./COM8",
    "interface": "jtag",
    "gdbPath": "C:/Develop_Software/gdb-multiarch-14.1/bin/gdb-multiarch.exe",
}
```
