nasm -o loader.bin loader.S -O0
nasm -o mbr.bin mbr.S

cp ./loader.bin ../../bochs/binfile
cp ./mbr.bin ../../bochs/binfile