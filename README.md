### 本项目是复现简单操作系统内核 参考《操作系统真象还原》

- 目前还在调试书写中


---
## debug 记录
> 第三章
> 
> 从mbr跳转至loader时没有跳过gdt 未能正确识别代码段
> ~~~assembly
> jmp LOADER_BASE_ADDR          ; 改成
> jmp LOADER_BASE_ADDR + 0x300
> ~~~
>
> - 第十一章 idt_init中
> ~~~c
> // 书上多了一个括号导致出错 导致无法运行用户线程
> uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
> ~~~
> 第十二章
>
> 系统调用时 未能正确读取参数列表 导致无法实现调用功能
> 
> 第十三章
>
> 1. printk 函数出错 没有给buf赋初值 导致输出有乱码
> 2. ide_init 中忘记初始化分区队列
> 3. ide_write和ide_read中未能正确初始化互斥锁
> 4. vsprintf函数未添加%c处理 导致部分输出有误 目前已添加 