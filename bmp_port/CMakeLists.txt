
sdk_inc(.)
sdk_src(platform.c)
sdk_src(cdc_acm.c)
# sdk_src(gdb_if.c)
sdk_src(swdptap.c)
sdk_src(jtagtap.c)
sdk_src(timing_hpm.c)
sdk_src(usb_serial.c)


sdk_inc(../blackmagic/src)
sdk_inc(../blackmagic/src/include)
sdk_inc(../blackmagic/src/target)

sdk_src(../blackmagic/src/command.c)
sdk_src(../blackmagic/src/crc32.c)
sdk_src(../blackmagic/src/exception.c)
sdk_src(../blackmagic/src/gdb_main.c)
sdk_src(../blackmagic/src/gdb_packet.c)
sdk_src(../blackmagic/src/hex_utils.c)
sdk_src(../blackmagic/src/maths_utils.c)
# sdk_src(../blackmagic/src/meson.build)
sdk_src(../blackmagic/src/morse.c)
sdk_src(../blackmagic/src/remote.c)
sdk_src(../blackmagic/src/remote.h)
sdk_src(../blackmagic/src/rtt.c)
sdk_src(../blackmagic/src/timing.c)

sdk_src(../blackmagic/src/target/adi.c)
sdk_src(../blackmagic/src/target/adiv5_jtag.c)
sdk_src(../blackmagic/src/target/adiv5_swd.c)
sdk_src(../blackmagic/src/target/adiv5.c)
sdk_src(../blackmagic/src/target/adiv6.c)
sdk_src(../blackmagic/src/target/apollo3.c)
sdk_src(../blackmagic/src/target/at32f43x.c)
sdk_src(../blackmagic/src/target/ch32f1.c)
sdk_src(../blackmagic/src/target/ch579.c)
sdk_src(../blackmagic/src/target/cortex.c)
sdk_src(../blackmagic/src/target/cortexar.c)
sdk_src(../blackmagic/src/target/cortexm.c)
sdk_src(../blackmagic/src/target/efm32.c)
sdk_src(../blackmagic/src/target/gdb_reg.c)
sdk_src(../blackmagic/src/target/hc32l110.c)
sdk_src(../blackmagic/src/target/imxrt.c)
sdk_src(../blackmagic/src/target/jtag_devs.c)
sdk_src(../blackmagic/src/target/jtag_scan.c)
sdk_src(../blackmagic/src/target/jtagtap_generic.c)
sdk_src(../blackmagic/src/target/kinetis.c)
sdk_src(../blackmagic/src/target/lmi.c)
sdk_src(../blackmagic/src/target/lpc_common.c)
sdk_src(../blackmagic/src/target/lpc11xx.c)
sdk_src(../blackmagic/src/target/lpc15xx.c)
sdk_src(../blackmagic/src/target/lpc17xx.c)
sdk_src(../blackmagic/src/target/lpc40xx.c)
sdk_src(../blackmagic/src/target/lpc43xx.c)
sdk_src(../blackmagic/src/target/lpc546xx.c)
sdk_src(../blackmagic/src/target/lpc55xx.c)
sdk_src(../blackmagic/src/target/msp432e4.c)
sdk_src(../blackmagic/src/target/msp432p4.c)
sdk_src(../blackmagic/src/target/mspm0.c)
sdk_src(../blackmagic/src/target/nrf51.c)
sdk_src(../blackmagic/src/target/nrf54l.c)
sdk_src(../blackmagic/src/target/nrf91.c)
sdk_src(../blackmagic/src/target/nxpke04.c)
sdk_src(../blackmagic/src/target/puya.c)
sdk_src(../blackmagic/src/target/renesas_ra.c)
sdk_src(../blackmagic/src/target/renesas_rz.c)
sdk_src(../blackmagic/src/target/riscv_adi_dtm.c)
sdk_src(../blackmagic/src/target/riscv_debug.c)
sdk_src(../blackmagic/src/target/riscv_jtag_dtm.c)
sdk_src(../blackmagic/src/target/riscv32.c)
sdk_src(../blackmagic/src/target/riscv64.c)
sdk_src(../blackmagic/src/target/rp2040.c)
sdk_src(../blackmagic/src/target/rp2350.c)
sdk_src(../blackmagic/src/target/s32k3xx.c)
sdk_src(../blackmagic/src/target/sam3x.c)
sdk_src(../blackmagic/src/target/sam4l.c)
sdk_src(../blackmagic/src/target/samd.c)
sdk_src(../blackmagic/src/target/samx5x.c)
sdk_src(../blackmagic/src/target/semihosting.c)
sdk_src(../blackmagic/src/target/sfdp.c)
sdk_src(../blackmagic/src/target/spi.c)
sdk_src(../blackmagic/src/target/stm32_common.c)
sdk_src(../blackmagic/src/target/stm32f1.c)
sdk_src(../blackmagic/src/target/stm32f4.c)
sdk_src(../blackmagic/src/target/stm32g0.c)
sdk_src(../blackmagic/src/target/stm32h5.c)
sdk_src(../blackmagic/src/target/stm32h7.c)
sdk_src(../blackmagic/src/target/stm32l0.c)
sdk_src(../blackmagic/src/target/stm32l4.c)
sdk_src(../blackmagic/src/target/stm32mp15.c)
sdk_src(../blackmagic/src/target/stm32wb0.c)
# sdk_src(../blackmagic/src/target/swdptap_generic.c)
sdk_src(../blackmagic/src/target/target_flash.c)
sdk_src(../blackmagic/src/target/target_probe.c)
sdk_src(../blackmagic/src/target/target.c)
sdk_src(../blackmagic/src/target/zynq7000.c)
sdk_src(../blackmagic/src/target/hpm_xpi.c)


sdk_compile_definitions("-DCONFIG_BMDA=0")
sdk_compile_definitions("-DCONFIG_POINTER_SIZE=4")
sdk_compile_definitions("-DNO_LIBOPENCM3")
sdk_compile_definitions("-DUSB_HS")
sdk_compile_definitions("-DADVERTISE_NOACKMODE=1")
sdk_compile_definitions("-DCONFIG_RISCV=1")
sdk_compile_definitions("-DCONFIG_RISCV_ACCEL=1")
# sdk_compile_definitions("-DENABLE_DEBUG=1")
