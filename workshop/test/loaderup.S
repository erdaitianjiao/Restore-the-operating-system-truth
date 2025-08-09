%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR 		   ;是个程序都需要有栈区 我设置的0x600以下的区域到0x500区域都是可用空间 况且也用不到
jmp loader_start                     		   	   ;下面存放数据段 构建gdt 跳跃到下面的代码区 
				       		   ;对汇编再复习 db define byte,dw define word,dd define dword
    GDT_BASE        : dd 0x00000000          		   ;刚开始的段选择子0不能使用 故用两个双字 来填充
   		       dd 0x00000000 
    
    CODE_DESC       : dd 0x0000FFFF         		   ;FFFF是与其他的几部分相连接 形成0XFFFFF段界限
    		       dd DESC_CODE_HIGH4
    
    DATA_STACK_DESC : dd 0x0000FFFF
  		       dd DESC_DATA_HIGH4
    		       
    VIDEO_DESC      : dd 0x80000007         		   ;0xB8000 到0xBFFFF为文字模式显示内存 B只能在boot.inc中出现定义了 此处不够空间了 8000刚好够
                      dd DESC_VIDEO_HIGH4     	   ;0x0007 (bFFFF-b8000)/4k = 0x7
                 
    GDT_SIZE              equ $ - GDT_BASE               ;当前位置减去GDT_BASE的地址 等于GDT的大小
    GDT_LIMIT       	   equ GDT_SIZE - 1   	           ;SIZE - 1即为最大偏移量
    
    times 59 dq 0                             	   ;预留59个 define double四字型 8字描述符
    times 5 db 0                                         ;为了凑整数 0x800 导致前面少了三个
    
    total_mem_bytes  dd 0
    	               			           ;在此前经过计算程序内偏移量为0x200 我算了算 60*8+4*8=512 刚好是 0x200 说这里的之后还会用到
    							   ;我们刚开始程序设置的地址位置为 0x600 那这就是0x800
 
    
    gdt_ptr           dw GDT_LIMIT			   ;gdt指针 2字gdt界限放在前面 4字gdt地址放在后面 lgdt 48位格式 低位16位界限 高位32位起始地址
    		       dd GDT_BASE
    		       
    ards_buf times 244 db 0                              ;buf  记录内存大小的缓冲区
    ards_nr dw 0					   ;nr 记录20字节结构体个数  计算了一下 4+2+4+244+2=256 刚好256字节
    							   ;书籍作者有强迫症 哈哈 这里244的buf用不到那么多的 实属强迫症使然 哈哈
    
    SELECTOR_CODE        equ (0X0001<<3) + TI_GDT + RPL0    ;16位寄存器 4位TI RPL状态 GDT剩下的选择子
    SELECTOR_DATA	  equ (0X0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO       equ (0X0003<<3) + TI_GDT + RPL0   
    
loader_start:
    
    mov sp,LOADER_BASE_ADDR                                   ;先初始化了栈指针
    xor ebx,ebx                                               ;异或自己 即等于0
    mov ax,0                                       
    mov es,ax                                                 ;心有不安 还是把es给初始化一下
    mov di,ards_buf                                           ;di指向缓冲区位置
.e820_mem_get_loop:
    mov eax,0x0000E820                                            ;每次都需要初始化
    mov ecx,0x14
    mov edx,0x534d4150
    
    int 0x15                                                  ;调用了0x15中断
    jc  .e820_failed_so_try_e801                              ;这时候回去看了看jc跳转条件 就是CF位=1 carry flag = 1 中途失败了即跳转
    add di,cx							;把di的数值增加20 为了下一次作准备
    inc word [ards_nr]
    cmp ebx,0
    jne .e820_mem_get_loop                                    ;直至读取完全结束 则进入下面的处理时间
    
    mov cx,[ards_nr]                                          ;反正也就是5 cx足以
    mov ebx,ards_buf
    xor edx,edx
.find_max_mem_area:
    
    mov eax,[ebx]						 ;我也不是很清楚为什么用内存上限来表示操作系统可用部分
    add eax,[ebx+8]                                            ;既然作者这样用了 我们就这样用
    add ebx,20    						 ;简单的排序
    cmp edx,eax
    jge .next_ards
    mov edx,eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok
    
.e820_failed_so_try_e801:                                       ;地址段名字取的真的简单易懂 哈哈哈哈 
    mov ax,0xe801
    int 0x15
    jc .e801_failed_so_try_88
   
;1 先算出来低15MB的内存    
    mov cx,0x400
    mul cx                                                      ;低位放在ax 高位放在了dx
    shl edx,16                                                  ;dx把低位的16位以上的书往上面抬 变成正常的数
    and eax,0x0000FFFF                                          ;把除了16位以下的 16位以上的数清零 防止影响
    or edx,eax                                                  ;15MB以下的数 暂时放到了edx中
    add edx,0x100000                                            ;加了1MB 内存空缺 
    mov esi,edx
    
;2 接着算16MB以上的内存 字节为单位
    xor eax,eax
    mov ax,bx
    mov ecx,0x10000                                              ;0x10000为64KB  64*1024  
    mul ecx                                                      ;高32位为0 因为低32位即有4GB 故只用加eax
    mov edx,esi
    add edx,eax
    jmp .mem_get_ok
 
.e801_failed_so_try_88:
     mov ah,0x88
     int 0x15
     jc .error_hlt
     and eax,0x0000FFFF
     mov cx,0x400                                                 ;1024
     mul cx
     shl edx,16
     or edx,eax 
     add edx,0x100000

.error_hlt:
     jmp $
.mem_get_ok:
     mov [total_mem_bytes],edx
; --------------------------------- 设置进入保护模式 -----------------------------
; 1 打开A20 gate
; 2 加载gdt
; 3 将cr0 的 pe位置1
    
    in al,0x92                 ;端口号0x92 中 第1位变成1即可
    or al,0000_0010b
    out 0x92,al
   
    
    lgdt [gdt_ptr]
    
    
    mov eax,cr0                ;cr0寄存器第0位设置位1
    or  eax,0x00000001              
    mov cr0,eax
      
;-------------------------------- 已经打开保护模式 ---------------------------------------
    mov byte [gs:160],'P'
    jmp $
    jmp dword SELECTOR_CODE:p_mode_start                       ;刷新流水线
 

setup_page:
    mov ecx,0x1000                                             ;循环4096次 将页目录项清空 内存清0
    mov esi,0                                                   
    .clear_page_dir_mem:                                          ;dir directory 把页目录项清空
    mov byte [PAGE_DIR_TABLE_POS+esi],0
    inc esi
    loop .clear_page_dir_mem
    
    .create_pde: 
    mov eax,PAGE_DIR_TABLE_POS				  ;页目录项 起始位置
    add eax,0x1000                                              ;页目录项刚好4k字节 add eax即得第一个页表项的地址
                                                                ;接下来我们要做的是 把虚拟地址1M下和3G+1M 两部分的1M内存在页目录项中都映射到物理地址0-0XFFFFF
    or  eax, PG_P | PG_RW_W | PG_US_U                           ;哦 悟了 哈哈哈 这里设置为PG_US_U 是因为init在用户进程 如果这里设置成US_S 这样子连进内核都进不去了
        
    mov [PAGE_DIR_TABLE_POS+0x0],eax                             ;页目录项偏移0字节与偏移0xc00 对应0x 一条页目录项对应2^22位4MB 偏移由前10位*4字节得到 可自己推算一下
    mov [PAGE_DIR_TABLE_POS+0xc00],eax                        
    sub eax,0x1000      
    
    mov [PAGE_DIR_TABLE_POS+4092],eax                           ;虚拟内存最后一个目录项 指向页目录表自身 书上写的是为了动态操纵页表 我也不是很清楚 反正有用 先放放

;这里就创建了一页页表    
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    mov ecx,256
    mov esi,0
    mov ebx,PG_P | PG_RW_W | PG_US_U 
    
    .create_kernel_pte:           
    mov [eax+esi*4],ebx
    inc esi
    add ebx,0x1000
    loop .create_kernel_pte 
    
    
;这里对于我们这里填写的目录项所对应的页表 页表中我们还没填写的值
;为了实现 真正意义上的 内核空间被用户进程完全共享
;只是把页目录与页表的映射做出来了 

    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000       					   ;eax此时处于第二个页表
    or  eax,PG_P | PG_RW_W | PG_US_U
;这里循环254次可以来分析一下 我们这里做的是 0xc0 以上部分的映射    0xc0 对应的是第768个页表项 页表项中一共有 2^10=1024项
;第1023项我们已经设置成 映射到页目录项本身位置了 即1022 - 769 +1 = 254
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254						  
    mov esi,769
        
    .create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde 
    
    ret
        

 [bits 32]
 p_mode_start: 
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    
    mov byte [gs:160],'P'
    
    jmp $
