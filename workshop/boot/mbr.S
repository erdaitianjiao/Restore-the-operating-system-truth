; 主引导程序

%include "../include/boot.inc"

SECTION MBR vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax
    
    ; 利用0x06功能号进行清屏
    ; int 0x10 功能号 0x06  功能 上卷窗口
    ; 输入
    ; ah 功能号 0x06 
    ; al 上卷的行数 (如果为0 则表示全部上卷)
    ; bh 上卷行的属性
    ; (cl, ch) 窗口左上角位置
    ; (dl, dh) 窗口右下角位置
    ; 无返回值

    mov ax,0x600
    mov bx,0x700
    mov cx,0                        
    mov dx,0x184f       ; VGA文本模式中,一行只能容纳80个字符 共25行
                        ; 下标从0开始 所以0x18=24 0x4f=79                                  
    int 10h
    
    ; A表示绿色背景闪烁
    ; 4表示前景色为红色
    ;输出字符串 "1MBR"

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

    mov eax, LOADER_START_SECTOR    ; 起始位置LBA位置
    mov bx, LOADER_BASE_ADDR        ; 读取的位置
    mov cx, 4                       ; 待读取的扇区数
    call rd_disk_m_16               ; 读取扇区的起始部分 (一个扇区)

    jmp LOADER_BASE_ADDR + 0x300

; 从磁盘读取n个扇区
rd_disk_m_16:

    ; eax LBA 扇区号
    ; bx  将数据写入的内存地址
    ; cx  读入的扇区数量

    mov esi, eax
    mov di, cx                      ; 备份eax 和 cx

    ; 读写磁盘
    ; 1 设置要读取的扇区数量
    
    mov dx, 0x1f2
    mov al, cl
    out dx, al                      ; 数量

    mov eax, esi                    ; 恢复ax
    
    ; 2 将LBA地址存入0x1f3 - 0x1f6 
    
    ; LBA 7 - 0 位写入端口 0x1f3
    mov dx, 0x1f3
    out dx, al

    ; LBA 15 - 8 位写入端口 0x1f4
    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    ;LBA 23 - 16 位写入端口 0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    
    shr eax, cl
    and al, 0x0f                   ; LBA 第 24 - 27 位
    or  al, 0xe0                   ; 设置 7 - 4 位为1110,表示lba模式
    mov dx, 0x1f6
    out dx, al
    
    ; 3 向 0x1f7 端口写入读命令 0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ; 4 检测硬盘状态
.not_ready:
    ; 同一端口 写时表示写入命令字 读时表示读入硬盘状态
    nop
    in al, dx
    and al, 0x88                    ; 第4位为1表示硬盘控制器已准备好数据传输
                                    ; 第7位为1表示硬盘忙
    cmp al, 0x08
    jnz .not_ready                  ; 没有准备好就继续等待

    ; 5 从 0x1f0 端口读数据
    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    ; di为要读取的扇区数 一个扇区有512字节 每次读入一个字 
    ; 共需di*512/2次 所以di*256
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

times 510-($-$$) db 0
db 0x55, 0xaa