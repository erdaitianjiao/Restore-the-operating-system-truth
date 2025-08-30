### 本项目是复现简单操作系统内核 参考《操作系统真象还原》

- 目前还在调试书写中


- debug 记录

 > 第十一章 idt_init中
 > ~~~c
  > // 书上多了一个括号导致出错
 > uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
 > ~~~