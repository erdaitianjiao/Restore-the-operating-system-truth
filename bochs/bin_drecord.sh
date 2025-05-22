dd if=./binfile/mbr.bin of=./hd60M.img bs=512 count=1 conv=notrunc
dd if=./binfile/loader.bin of=./hd60M.img bs=512 count=2 seek=2 conv=notrunc