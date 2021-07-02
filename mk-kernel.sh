#!/bin/bash
path=/home/work/riscv-chip/tina-d1-v1.01
make  ARCH=riscv  CROSS_COMPILE=${path}/lichee/brandy-2.0/tools/toolchain/riscv64-linux-x86_64-20200528/bin/riscv64-unknown-linux-gnu-  \
&& ./rvboards_img/mkbootimg  --kernel  arch/riscv/boot//Image --board  d1-nezha --base  0x40200000 --kernel_offset  0x0 -o  ./rvboards_img/rvboards_boot.img
