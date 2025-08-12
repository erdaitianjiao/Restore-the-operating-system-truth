TI_GDT equ 00b
RPL0   equ 000b
SELECTOR_VIDEO  equ (0x0003 << 3) + TI_GDT + RPL0
SELECTOR_DATA   equ (0x0002 << 3) + TI_GDT + RPL0

[bits 32]
section .text

;-------------function name: put_str---------------
;function describe: put string into chosed situation
global put_str
put_str:
   

;-------------function name: put_char---------------
;function describe: put char into chosed situation
global put_char
put_char:
    pushad                      ;back up of 32 bits registers
    ;gs point to video register
    ;cant direcly mov gs
    mov ax, SELECTOR_VIDEO      
    mov gs, ax

    ;get now situation of mouse
    ;high 8 bits
    mov dx, 0x03d4              ;point register
    mov al, 0x0e                ;used to privide high 8 bits
    out dx, al
    mov dx, 0x03d5              ;set 0x3d5 to get or set mouse situation
    in  al, dx                  ;get high 8 bits
    shl ax, 0x8

    ;low  8bits
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in  al, dx
    
    ;save in bx
    mov bx, ax
    
    ;get char from statk
    mov byte cl, [esp + 36]
                            ;pushad 4 * 8 = 32 bits
                            ;return 4 bits, equal 36 bits
                            ;cr = 0x0d, lf = 0x0a
    cmp cl, 0xd
    jz .is_carriage_return
    cmp cl, 0xa
    jz .is_line_feed
    cmp cl, 0x8
    jz .is_backspace

    jmp .put_other

;deal with backspace
.is_backspace:
    cmp bx, 0
    je .set_cursor
    dec bx                  
    shl bx, 1               ;bx * 2

    mov word [gs: bx], 0x0720
    shr bx, 1
    jmp .set_cursor

.put_other:
   shl bx, 1
    
   mov byte [gs: bx], cl
   inc bx
   mov byte [gs: bx], 0x07
   inc bx
   shr bx, 1
   cmp bx, 2000
   jl .set_cursor

.is_line_feed:
.is_carriage_return:
    xor dx, dx
    mov ax, bx
    mov si, 80

    div si
    sub bx, dx

.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000

.is_line_feed_end:
    jl .set_cursor

.roll_screen:
    cld
    mov ax, SELECTOR_DATA
    mov es, ax
    mov di, es
    mov ecx, 920

    mov esi, 0xc00b80a0
    mov edi, 0xc00b8000
    rep movsd

    mov ebx, 3840
    mov ecx, 80
    mov esi, 0

.cls:
    mov word [ebx + esi * 2], 0x720
    inc esi

    loop .cls
    mov ebx, 1920

.set_cursor:
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_done:
    popad
    ret