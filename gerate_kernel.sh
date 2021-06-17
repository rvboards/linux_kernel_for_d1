#!/bin/bash
path=$(pwd)
cp ${path}/lichee/linux-5.4/arch/riscv/boot/Image ${path}/out/d1-nezha/compile_dir/target/linux-d1-nezha/Image && \
${path}/out/d1-nezha/compile_dir/host/pack-bintools/mkbootimg --kernel  ${path}/out/d1-nezha/compile_dir/target/linux-d1-nezha/Image --board  d1-nezha --base  0x40200000 --kernel_offset  0x0 -o  ${path}/out/d1-nezha/d1-nezha-boot.img && \
	cp -fpR ${path}/out/d1-nezha/d1-nezha-boot.img ${path}/out/d1-nezha/boot.img

