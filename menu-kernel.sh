#!/bin/bash
path=/home/work/riscv-chip/tina-d1-v1.01
make menuconfig ARCH=riscv CROSS_COMPILE=${path}/lichee/brandy-2.0/tools/toolchain/riscv64-linux-x86_64-20200528/bin/riscv64-unknown-linux-gnu-
