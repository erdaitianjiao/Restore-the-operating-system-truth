### 本项目是复现简单操作系统内核 参考《操作系统真象还原》

- 目前还在调试书写中


---
## Debug 记录
> 第三章
> 
> 从mbr跳转至loader时没有跳过gdt 未能正确识别代码段
> ~~~assembly
> jmp LOADER_BASE_ADDR          ; 改成
> jmp LOADER_BASE_ADDR + 0x300
> ~~~
> 第四章
>
> 由于进入保护模式后变成32位了<br>
> 汇编中未添加伪代码`[bits 32]`<br>
> 导致汇编出的机器码反汇编和源程序不一样 导致错误
>
> 第九章
>
> 由于上一章测试键盘的时候把time定时中断屏蔽<br>
> 无法正常调用中断函数进行`sechulehl`函数进行线程调度<br>
> 在`interrupt.c`中的`pic_init中`同时开启键盘和time中断既可 
> 
> 第十一章 idt_init中
> ~~~c
> // 书上多了一个括号导致出错 导致无法运行用户线程
> uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
> ~~~
> 第十二章
>
> 系统调用时 未能正确读取参数列表 导致无法实现`printf`调用功能
> 
> 第十三章
>
> 1. printk 函数出错 没有给buf赋初值 导致输出有乱码
> 2. ide_init 中忘记初始化分区队列
> 3. ide_write和ide_read中未能正确初始化互斥锁
> 4. vsprintf函数未添加%c处理 导致部分输出有误 目前已添加 