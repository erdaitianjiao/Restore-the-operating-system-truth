%include "boot.inc"
section push32_test vstart=0x900
jmp LOADER_START_SECTOR

gdt_addr:
    ;create the gdt
    GDT_BASE: dd 0x00000000
              dd 0x00000000

    CODE_DESC: dd 0x0000FFFF
               dd DECE_CODE_HIGH4
    
    DATA_STACK_DESC: dd 0x0000FFFF
                     dd DECE_DATA_HIGH4

    VIDEO_DESC: dd 0x80000008
                dd DESC_VIDEO_HIGH$ ; now the dpl1 is setted to 0
    
    GDT_SIZE equ $ - GPT_BASE
    GDT_LIMIT equ GDT_SIZE - 1
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPLO
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0

    gdt_ptr: dw GDT_LIMIT
             dd gdt_addr

    loader_start:

;------------ready to get into protected mod------------;

    ;first step: switch on A20
    ;second step: load gdt
    ;third step: set cr0 pe to 1
    ;-----------------switch on A20-----------------;
    
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ;-----------------load GDT----------------------;
    
    lgdt [gdt_ptr]

    ;------------------set cr0 zero to 1
    
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ;reflash the stream, avoid

    jmp SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x900
    push byte 0x7
    push word 0x8
    push dword 0x9
    jmp $

