# Instructions

注：rvboards镜像请更新到v0.5版本以上

## 生成kernel 镜像rvboards_boot.img

1、执行 ./mk-kernel.sh 编译kernel，并生成rvboards_img/rvboards_boot.img

​	配置默认的config 和board.dts

1. config
   1. cp config_rvboards_d1 .config
2. rvboards_dts
   1. cd arch/riscv/boot/dts/sunxi/
   2. cp board.dts_rvboards_d1 board.dts

2、下载交叉编译工具链，并配置mk-kernel.sh中CROSS_COMPILE

链接：https://pan.baidu.com/s/1-F9YPfm_dGFTlf-rld8OEw 
提取码：3o5v

注：交叉编译工具链从全志D1哪吒·Tina SDK中提取，目录如下
tina-d1-v1.01/lichee/brandy-2.0/tools/toolchain/riscv64-linux-x86_64-20200528.tar.xz

## 更换kernel 镜像rvboards_boot.img

```shell
#1、在桌面目录下新建boot文件夹
root@RVBoards:~/Desktop#  mkdir boot

#2、mount 挂载boot分区
root@RVBoards:~/Desktop# mount /dev/mmcblk0p4 boot/

#3、检查mount是否成功
root@RVBoards:~/Desktop# ls boot/
'System Volume Information'   boot_debian.img   config.txt   rt-smart
 boot.img                     boot_tina.img     overlay
 #4、把rvboards_boot.img放到boot目录下，配置config.txt
root@RVBoards:~/Desktop/boot# vim config.txt
	## kernel=rvboards_boot.img
 #5、重启,使rvboards_boot.img生效
root@RVBoards:~/Desktop/boot# reboot
 
```





