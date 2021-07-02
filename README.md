# Instructions

Note: Please update the RVBoards image to V0.5 or above

## Generate the kernel image rvboards_boot.img
1 Compile the kernel with./mk-kernel.sh and generate rvboards_img/rvboards_boot.img
	Configure the default config and board.dts
	1. config
	   1. cp config_rvboards_d1 .config
	2. rvboards_dts
	   1. cd arch/riscv/boot/dts/sunxi/
	   2. cp board.dts_rvboards_d1 board.dts
2 Download the cross-compile toolchain and configure CROSS_COMPILE in mk-kernel.sh

Baidu network disk link:https://pan.baidu.com/s/1-F9YPfm_dGFTlf-rld8OEw 

Extract code:3o5v

Note:The cross-compilation toolchain is extracted from the Zhan D1 Nezha Tina SDK. The directory is as follows:
	<<tina-d1-open/lichee/brandy-2.0/tools/toolchain/riscv64-linux-x86_64-20200528.tar.xz>>
## Replace the kernel image rvboards_boot.img
```shell
 #1. Create a new Boot folder in the desktop directory
root@RVBoards:~/Desktop#  mkdir boot

 #2 mount the boot partition
root@RVBoards:~/Desktop# mount /dev/mmcblk0p4 boot/

 #3 Check if mount was successful
root@RVBoards:~/Desktop# ls boot/
'System Volume Information'   boot_debian.img   config.txt   rt-smart
 boot.img                     boot_tina.img     overlay
 
 #4. Place rvboards_boot.img in the boot directory and configure config.txt
root@RVBoards:~/Desktop/boot# vim config.txt
	## kernel=rvboards_boot.img
  
 #5. Restart to rvboards_boot.img
root@RVBoards:~/Desktop/boot# reboot

```

