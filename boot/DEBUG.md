# UF2 Bootloader 调试指南

## 问题：设备没有枚举

如果烧录bootloader后设备没有枚举，按以下步骤排查：

## 1. 检查串口输出

bootloader启动后会通过UART输出调试信息：

```
========================================
  HPM UF2 Bootloader
  Version: 1.0.0
  Build: Apr 15 2026 16:53:21
  Device: HPM5301
========================================
Board initialization complete
Checking bootloader entry conditions...
Staying in bootloader mode
Initializing USB MSC+DFU...
USB initialized, waiting for connection...
========================================
```

### 连接串口

```bash
# 使用minicom
sudo minicom -D /dev/ttyUSB0 -b 115200

# 或使用screen
sudo screen /dev/ttyUSB0 115200

# 或使用picocom
picocom -b 115200 /dev/ttyUSB0
```

**如果看不到输出**:
- 检查UART引脚是否正确连接（TX/RX/GND）
- 确认波特率为 115200
- 检查board_init_console()是否正常工作

## 2. 检查烧录地址

Bootloader **必须**烧录到 Flash 起始地址 `0x80000000`：

```bash
# 使用OpenOCD烧录
openocd -f your_config.cfg \
  -c "program boot/build/output/hpm-uf2-boot.elf 0x80000000 verify reset exit"

# 或者使用bin文件
openocd -f your_config.cfg \
  -c "program boot/build/output/hpm-uf2-boot.bin 0x80000000 verify reset exit"
```

**错误的烧录地址会导致**:
- 设备无法启动
- 没有任何串口输出
- USB不枚举

## 3. 检查Boot配置

HPM5301的Flash XIP配置必须正确：

### 检查nor_cfg_option

在flash_boot.ld中定义：
```
.nor_cfg_option __nor_cfg_option_load_addr__ : {
    KEEP(*(.nor_cfg_option))
} > XPI0
```

### 检查boot_header

```
.boot_header __boot_header_load_addr__ : {
    __boot_header_start__ = .;
    KEEP(*(.boot_header))
    KEEP(*(.fw_info_table))
    KEEP(*(.dc_info))
    __boot_header_end__ = .;
} > XPI0
```

## 4. 检查时钟配置

确保以下函数被正确调用（已在最新代码中修复）：

```c
void boot_port_board_init(void)
{
    board_init_usb_dp_dm_pins();  // ✅ 必须第一个
    board_init_clock();            // ✅ 修复了拼写错误
    board_init_console();          // ✅
    board_init_led_pins();         // ✅
    board_init_usb(HPM_USB0);      // ✅ 新增
}
```

## 5. 检查USB配置

### 5.1 查看USB时钟

```c
// 在board_init_usb()中会自动配置：
clock_add_to_group(clock_usb0, 0);
```

### 5.2 检查USB PHY配置

```c
usb_phy_using_internal_vbus(HPM_USB0);  // 使用内部VBUS
usb_phy_disable_dp_dm_pulldown(HPM_USB0);  // 禁用DP/DM下拉
```

### 5.3 检查USB引脚

确认USB D+/D-引脚正确配置（在board.c中）。

## 6. 使用调试器检查

### 6.1 检查PC寄存器

```gdb
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) info registers pc
```

PC应该指向 0x80000000 附近。

### 6.2 检查main函数是否执行

```gdb
(gdb) break main
(gdb) continue
```

如果断点命中，说明程序正常运行。

### 6.3 检查USB初始化

```gdb
(gdb) break msc_bootuf2_init
(gdb) continue
```

## 7. 常见问题及解决

### 问题1: 设备立即跳转到应用区

**现象**: Bootloader输出 "No bootloader request, jumping to application..."

**原因**: 
- 应用区有有效固件
- Boot引脚未按下
- 没有触发强制进入bootloader

**解决**:
```c
// 方法1: 按住Boot按钮上电

// 方法2: 在应用中写入magic值
*(volatile uint32_t *)0xF0400000 = 0x424F4F54;  // "BOOT"
NVIC_SystemReset();

// 方法3: 临时修改代码强制进入
// 在boot_port_check_bootloader_request()开头添加：
return true;  // 强制进入bootloader
```

### 问题2: USB枚举失败

**检查清单**:
- [ ] USB时钟已启用
- [ ] USB PHY已配置
- [ ] DP/DM引脚已配置
- [ ] VBUS电源正常（5V）
- [ ] D+ 上有1.5K上拉电阻（USB FS必需）

**测试USB电气特性**:
```bash
# Linux下查看USB设备
lsusb -v

# 查看dmesg输出
sudo dmesg -w
# 应该看到类似：
# usb 1-1: new full-speed USB device number X using xhci_hcd
```

### 问题3: 串口无输出

**可能原因**:
1. 时钟未初始化 → `board_init_clock()` 拼写错误（已修复）
2. UART引脚未配置
3. 波特率不匹配
4. TX/RX接反

**验证**:
```bash
# 测试回环
# 将TX和RX短接，发送应该能收到
echo "test" > /dev/ttyUSB0
cat /dev/ttyUSB0
```

### 问题4: LED不亮

```c
// 检查LED初始化
board_init_led_pins();
board_led_write(1);  // 1=ON, 0=OFF

// 手动测试LED
gpio_write_pin(BOARD_LED_GPIO_CTRL, 
               BOARD_LED_GPIO_INDEX, 
               BOARD_LED_GPIO_PIN, 1);
```

## 8. 逐步调试流程

### Step 1: 确认程序运行

添加无限循环：
```c
int main(void) {
    while(1);  // 如果程序运行，调试器应该停在这里
}
```

### Step 2: 确认串口工作

```c
int main(void) {
    board_init_clock();
    board_init_console();
    while(1) {
        printf("Hello\r\n");
        board_delay_ms(1000);
    }
}
```

### Step 3: 确认USB初始化

```c
int main(void) {
    boot_port_board_init();
    printf("Init done\r\n");
    msc_bootuf2_init(0, HPM_USB_BASE);
    printf("USB init done\r\n");
    while(1);
}
```

### Step 4: 完整功能测试

使用完整的bootloader代码。

## 9. 重新编译bootloader

最新修复已包含在代码中，重新编译：

```bash
cd boot
./build_bootloader.sh --clean
```

## 10. 硬件检查清单

- [ ] 电源供电正常（3.3V核心，5V USB）
- [ ] 晶振工作正常（24MHz）
- [ ] BOOT0配置正确（从Flash启动）
- [ ] USB连接器正常
- [ ] UART连接正常
- [ ] 调试接口（SWD/JTAG）正常
- [ ] LED指示灯连接正常

## 11. 获取更多帮助

如果以上步骤都无效，请提供以下信息：

1. **串口完整输出**（如果有）
2. **OpenOCD烧录日志**
3. **lsusb输出**（Linux）或设备管理器截图（Windows）
4. **dmesg最后50行**（Linux）
5. **电路板照片**（标注USB/UART/电源连接）
6. **使用的具体硬件型号**

## 12. 快速测试命令

```bash
# 1. 重新编译
cd boot && ./build_bootloader.sh

# 2. 烧录
openocd -f openocd.cfg \
  -c "program build/output/hpm-uf2-boot.elf verify reset exit"

# 3. 监控串口
picocom -b 115200 /dev/ttyUSB0

# 4. 监控USB
sudo dmesg -w
# 另一个终端
lsusb -v | grep -A 10 "idVendor.*idProduct"
```

## 最新修复（2026-04-15）

✅ 修复了 `board_iwdwnit_clock()` 拼写错误 → `board_init_clock()`
✅ 添加了 `board_init_usb()` 调用
✅ 调整了初始化顺序（USB pins → Clock → USB controller）
✅ 增加了详细的调试输出
✅ 优化了错误处理流程

请重新编译并烧录测试！
