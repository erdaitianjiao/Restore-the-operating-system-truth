#ifndef _KERNEL_INTERRUPT_H
#define _KERNEL_INTERRUPT_H
#include "stdint.h"

typedef void* intr_handler;
void idt_init(void);

// 定义中断两种状态
// INTR_OFF 为 0 表示关中断
// INTR_ON  为 1 表示开中断
enum intr_status {

    INTR_OFF,               // 中断关闭
    INTR_ON                 // 中断打卡

};

enum intr_status intr_eabale(void);
enum intr_status intr_disable(void);
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_get_status(void);

#endif
