TI_GDT equ 00b
RPL0   equ 000b
SELECTOR_VIDEO  equ (0x0003 << 3) + TI_GDT + RPL0
SELECTOR_DATA   equ (0x0002 << 3) + TI_GDT + RPL0


section .data

put_int_buffer dq 0                       ; 定义8字节缓冲区得到数字到字符的转换

[bits 32]
section .text
; -------------函数名称 put_str---------------
; put_str 通过put_char来打印以0字符结尾的字符串
global put_str
put_str:
    ; 由于本函数中只用到了ebx和ecx,只备份这两个寄存器
    push ebx
    push ecx
    xor ecx, ecx                    ; 准备用ecx存储参数 清空
    mov ebx, [esp + 12]             ; 从栈中得到待打印的字符串地址 
.goon:
    mov cl, [ebx]
    cmp cl, 0                       ; 如果处理到了字符串尾 跳到结束处返回
    jz .str_over
    push ecx                        ; 为put_char函数传递参数
    call put_char
    add esp, 4                      ; 跳过返回值
    inc ebx                         ; 指向下一个字符
    jmp .goon
.str_over:
   pop ecx
   pop ebx
   ret

;-------------函数名 put_char---------------
; 功能描述:把栈中的1个字符写入光标所在处
global put_char
put_char:

    pushad                          ; 备份32位寄存器环境
    ; 需要保证gs中为正确的视频段选择子 为保险起见 每次打印时都为gs赋值

    mov ax, SELECTOR_VIDEO      
    mov gs, ax

    ; 获取当前光标位置
    ; 先获得高8位
    mov dx, 0x03d4                  ; 索引寄存器
    mov al, 0x0e                    ; 用于提供光标位置的高8位
    out dx, al
    mov dx, 0x03d5                  ; 通过读写数据端口0x3d5来获得或设置光标位置 
    in  al, dx                      ; 得到了光标位置的高8位
    mov ah, al

    ; 再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in  al, dx
    
    ; 将光标存入bx
    mov bx, ax
    
    ; 下面这行是在栈中获取待打印的字符
    mov ecx, [esp + 36]
                                    ; pushad压入4×8＝32字节,加上主调函数的返回地址4字节,故esp+36字节
                                    ; cr = 0x0d, lf = 0x0a
    cmp cl, 0xd
    jz .is_carriage_return

    cmp cl, 0xa
    jz .is_line_feed

    cmp cl, 0x8                     ; BS(backspace)的asc码是8
    jz .is_backspace

    jmp .put_other

    ; 处理backspace
.is_backspace:

    dec bx                  
    shl bx, 1                       ; bx * 2

    mov word [gs: bx], 0x0720       ; 将待删除的字节补为0或空格皆可
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1
    
    mov byte [gs: bx], cl
    inc bx
    mov byte [gs: bx], 0x07         ; 字符属性
    inc bx
    shr bx, 1                       ; 恢复老的光标值
    cmp bx, 2000                    ; 若光标值小于2000,表示未写到显存的最后,则去设置新的光标值
                                    ; 若超出屏幕字符数大小(2000)则换行处理
    jl .set_cursor

.is_line_feed:                      ; 是换行符LF(\n)
.is_carriage_return:                ; 是回车符CR(\r)
   
    ; 如果是CR(\r),只要把光标移到行首就行了。                                
    xor dx, dx
    mov ax, bx
    mov si, 80

    div si
    sub bx, dx
                                    ; 以上4行处理\r的代码
.is_carriage_return_end:            ; 回车符CR处理结束
    add bx, 80
    cmp bx, 2000

.is_line_feed_end:                  ; 若是LF(\n),将光标移+80便可
    jl .set_cursor

   ;屏幕行范围是0-24 滚屏的原理是将屏幕的1-24行搬运到0-23行 再将第24行用空格填充
.roll_screen:                       ; 若超出屏幕大小，开始滚屏
    cld
    mov ax, SELECTOR_DATA
    mov es, ax
    mov di, es
    mov ecx, 960                    ; 一共有2000-80=1920个字符要搬运,共1920*2=3840字节.一次搬4字节,共3840/4=960次

    mov esi, 0xc00b80a0             ; 第1行行首
    mov edi, 0xc00b8000             ; 第0行行首
    rep movsd

    ; 将最后一行填充为空白
    mov ebx, 3840
    mov ecx, 80

.cls:
    mov word [gs: ebx], 0x720 ; 0x0720是黑底白字的空格键
    add ebx, 2

    loop .cls
    mov ebx, 1920                   ; 将光标值重置为1920,最后一行的首字符.

    ;将光标设为bx值
.set_cursor:
    ; 1 先设置高8位
    mov dx, 0x03d4                  ; 索引寄存器
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5                  ; 通过读写数据端口0x3d5来获得或设置光标位置
    mov al, bh
    out dx, al

    ;2 再设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_done:
    popad
    ret

; 将小端字节序的数字变成对应的ascii后，倒置
; 输入：栈中参数为待打印的数字
; 输出：在屏幕上打印16进制数字
global put_int
put_int:
   pushad
   mov ebp, esp
   mov eax, [ebp + 4 * 9]              ; call的返回地址占4字节+pushad的8个4字节
   mov edx, eax
   mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
   mov ecx, 8                          ; 32位数字中 16进制数字的位数是8个
   mov ebx, put_int_buffer 
; 将32位数字按照16进制的形式从低位到高位逐个处理 共处理8个16进制数字
.16based_4bits:                        ; 每4位二进制是16进制数字的1位 遍历每一位16进制数字
   and edx, 0x0000000F                 ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
   cmp edx, 9                          ; 数字0 - 9和a - f需要分别处理成对应的字符
   jg .is_A2F
   add edx, '0'                        ; 变成 0 - 9
   jmp .store

.is_A2F:
   sub edx, 10 
   add edx, 'A'                        ; 变成 A - F

   ; 将转换后的数字存储到 put_int_buffer 里面
.store:
   ; 此时dl中应该是数字对应的字符的ascii码
   mov [ebx + edi], dl
   dec edi
   shr eax, 4
   mov edx, eax
   loop .16based_4bits

.ready_to_print:
   inc edi

   ; 去除前导零
.skip_prefix_0:
   cmp edi, 8
   je .full0

.go_on_skip:
   mov cl, [put_int_buffer + edi]
   inc edi
   cmp cl, '0'
   je .skip_prefix_0             ; 继续判断下一位字符是否为字符0 (不是数字0)
   dec edi
   jmp .put_each_num

.full0:
   mov cl , '0'                  ; 都是0直接输出0

.put_each_num:
   push ecx                            ; 此时cl中为可打印的字符
   call put_char
   add esp, 4
   inc edi                             ; 使edi指向下一个字符
   mov cl, [put_int_buffer + edi]      ; 获取下一个字符到cl寄存器
   cmp edi, 8
   jl .put_each_num
   popad 
   ret
