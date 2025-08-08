nasm -f elf -o lib/kernel/print.o lib/kernel/print.S
gcc -m32 -I lib/kernel/ -c -o kernel/main.o kernel/main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel/kernel.bin kernel/main.o lib/kernel/print.o

cp kernel/kernel.bin ../bochs/binfile