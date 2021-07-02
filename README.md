# Instructions

Note: Please update the RVBoards image to V0.5 or above

## Generate the kernel image rvboards_boot.img
Compile the kernel with./mk-kernel.sh and generate rvboards_img/rvboards_boot.img
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

