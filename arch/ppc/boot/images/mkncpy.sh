# simple script to mkimage on the ramdisk and copy uImage and the resulting uboot-wrapped ramdisk to /mnt/samba

mkimage -n '2.6.11 Ramdisk' -A ppc -O linux -T ramdisk -C gzip -d ramdisk.image.gz ramdisk.img
cp uImage /mnt/samba/kernel.img
cp ramdisk.img /mnt/samba/.
