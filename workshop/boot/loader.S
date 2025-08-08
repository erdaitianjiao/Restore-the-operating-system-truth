%include "../include/boot.inc"

section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ  LOADER_BASE_ADDR
jmp loader_start


;create GDT and internal descriptor

    GDT_BASE:   dd 0x00000000
                dd 0x00000000

    CODE_DESC:  dd 0x0000FFFF
                dd DESC_CODE_HIGH4

    DATA_STACK_DESC:    dd 0x0000FFFF
                        dd DESC_DATA_HIGH4

    VIDEO_DESC:         dd 0x80000007             ;limit = (0xbffff - 0xb8000) / 4k = 0x7
                        dd DESC_VIDEO_HIGH4         ;now dpl is 0

    GDT_SIZE   equ $ - GDT_BASE
    GDT_LIMIT  equ GDT_SIZE - 1
    times 59   dq  0                            ;here to reserve 59 bits   
    times 5    db  0                            ;for whole number

    ;total_membyte used tp save capicatiy of mem
    ;the capicaty of loader is 0x200 
    ;loader.bin is loaded at 0x900
    ;so total_mem_byte is 0xb00
    total_mem_bytes dd 0x0000000

    SELECTOR_CODE  equ (0x0001 << 3) + TI_GDT + RPL0
    ;equal to (CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0

    SELECTOR_DATA  equ (0x0002 << 3) + TI_GDT + RPL0 

    SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0 

    ;below is gdt pointer ,first two bits is the range of GDT, last four bits is the start address of GDT

    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    ;loadermsg db '2 loader in real.'
    
    ;total_mem_bytes4 + gdt_ptr6+ards_buf244 + ards_nr2 = 256
    ards_buf times 244 db 0
    ards_nr  dw 0                                    ;used to record number of struct ards

loader_start:

    xor ebx, 20
    mov edx, 0x534d4150
    mov di,  ards_buf

.e820_mem_get_loop:
    mov eax, 0x0000e820
    mov ecx, 20

    int 0x15
    jc .e820_failed_so_try_e801
    add di, cx
    inc word [ards_nr]
    cmp eax, 0
    jnz .e820_mem_get_loop

    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx

.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx + 8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

.e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_try_88
    
    mov cx, 0x400
    mul cx
    shl edx, 16
    and eax, 0x0000ffff
    or  edx, eax
    add edx, 0x100000
    mov esi, edx
    
    xor eax, eax
    mov ax, bx
    mov ecx, 0x10000
    mul ecx
    mov edx, esi
    add edx, eax
    jmp .mem_get_ok

.e801_failed_so_try_88:
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0x0000ffff
    mov cx, 0x400

    mul cx
    shl edx, 16
    or edx, eax
    add edx, 0x100000

.error_hlt:
    jmp $
.mem_get_ok:
    mov [total_mem_bytes], edx

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