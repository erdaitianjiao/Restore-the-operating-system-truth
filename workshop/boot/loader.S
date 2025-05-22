%include "include/boot.inc"

section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ  LOADER_BASE_ADDR
jmp loader_start


;create GDT and internal descriptor

GDT_BASE:   dd 0x00000000
            dd 0x00000000

CODE_DESC:  dd 0x0000FFFF
            dd DESC_CODE_HIGH4

DATA_STACK_DESC:  dd 0x0000FFFF
                  dd DESC_DATA_HIGH4

VIDEO_DESC:       dd 0x80000007; limit = (0xbffff - 0xb8000) / 4k = 0x7
                  dd DESC_VIDEO_HIGH4; now dpl is 0

GDT_SIZE   equ $ - GDT_BASE
GDT_LIMIT  equ GDT_SIZE - 1
times 60   dq  0            ;here to reserve 60 bits

SELECTOR_CODE  equ (0x0001<<3) + TI_GDT + RPL0
;equal to (CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0

SELECTOR_DATA  equ (0x0002<<3) + TI_GDT + RPL0 

SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0 

;below is gdt pointer ,first two bits is the range of GDT, last four bits is the start address of GDT

gdt_ptr dw GDT_LIMIT
        dd GDT_BASE
loadermsg db '2 loader in real.$'

loader_start:
    
    ;INT 0x10 function number: 0x13 function describe: print string
    
    ;input:
    ;AH function number = 13H
    ;BH = number of page
    ;BL = propety (if AL == 00H or 01H)
    ;CX = length of string
    ;(DH, DL) = site(row, column)
    ;ES: BP = the address of output
    ;AL = the way to output
    ;   0 -- only string, the propety is outputed to BL
    ;        after displayed, cursor situation unchanged
    ;   1 -- only string, the propety is outputed to BL
    ;        after displayed, cursor situation changed
    ;   2 -- dispaly string and propety, cursor situation unchanged
    ;   3 -- dispaly string and propety, cursor situation changed
    ;no return value
    mov sp, LOADER_BASE_ADDR
    mov bp, loadermsg           ;ES: BP = the address of output
    mov cx, 22                  ;CX = length of string
    mov ax, cs
    mov es, ax
    mov ax, 0x1301              ;AH = 13, AL = 01h
    mov bx, 0x001f              ;page is 0(BH = 0) blue background and red front(BL = 1fh)
    mov dx, 0x1800
    int 0x10

;-------------------------test for pmode------------------------------;

 


;------------ready to get into protected mod------------;

    ;first step: switch on A20
    ;second step: load gdt
    ;third step: set cr0 pe to 1

    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ;-----------------load GDT----------------------;
    
    lgdt [gdt_ptr]

    ;--------------set cr0 zero to 1------------;
    
    mov eax, cr0
    or  eax, 0x00000001
    mov cr0, eax

    jmp dword SELECTOR_CODE: p_mode_start       ;reflash stream

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov byte [gs: 160], 'P'

    jmp $