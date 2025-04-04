;main boot program

%include "boot.inc"

SECTION MBR vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax
;clear device
;0x06 function to clear
    mov ax,0x600
    mov bx,0x700
    mov cx,0                        ;
    mov dx,0x184f
    int 10h
    
    ;A stand for Green background
    ;B stand for Red front color
    
    ;out put string "1MBR"

    mov byte [gs:0x00],'1'
    mov byte [gs:0x01],0xA4

    mov byte [gs:0x02],' '
    mov byte [gs:0x03],0xA4

    mov byte [gs:0x04],'M'
    mov byte [gs:0x05],0xA4

    mov byte [gs:0x06],'B'
    mov byte [gs:0x07],0xA4
        
    mov byte [gs:0x08],'R'
    mov byte [gs:0x09],0xA4

    mov eax, LOADER_START_SECTOR    ;start of lba
    mov bx, LOADER_BASE_ADDR        ;address of write ram
    mov cx, 1                       ;count of fans to write
    call rd_disk_m_16               ;next start address of fans

    jmp LOADER_BASE_ADDR
    
rd_disk_m_16:
    mov esi, eax
    mov di, cx                      ;backup for two index
    ;read and write disk
    ;first step: set the number of fans to write
    
    mov dx, 0x1f2
    mov al, cl
    out dx, al                      ;the number

    mov eax, esi                    ;get back ax
    
    ;second step: set address of LBA to 0x1f3 - 0x1f6
    
    ;LBA 7 - 0 bit to 0x1f3
    mov dx, 0x1f3
    out dx, al

    ;LBA 15 - 8 bit to 0x1f4
    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    ;LBA 23 - 16 bit to 0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    
    shr eax, cl
    and al, 0x0f                  ;LBA 24 - 27
    or  al, 0xe0                   ;set 7 - 4bit as 1110, stand for LBA mod
    mov dx, 0x1f6
    out dx, al
    
    ;third step: write read order to 0x1f7, 0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ;forth step: check up status of disk
.not_ready:
    ;as the same port
    nop
    in al, dx
    and al, 0x88                    ;forth bit as 1 stand for raddy seventh bit as 1 stand for busy

    cmp al, 0x08
    jnz .not_ready                  ;while not ready, keepping waiting

    ;fivth step: read data from port of 0x1f0
    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    ;di stand for the number of section, each section has 512 bits, each time read one bits, all need di*512/2 times, 
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

times 510-($-$$) db 0
db 0x55, 0xaa