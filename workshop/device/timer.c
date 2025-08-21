#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"

#define IRQ0_FREQUENCY 	        100
#define INPUT_FREQUENCY         1193180
#define COUNTER0_VALUE		    INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT		    0X40
#define COUNTER0_NO 		    0
#define COUNTER_MODE		    2
#define READ_WRITE_LATCH	    3
#define PIT_COUNTROL_PORT	    0x43

uint32_t ticks;                     // ticks是自中断开启以来总共的滴答数

// 把操作的计数器counter_no 读写属性rwl 计数器模式
// counter_mode写入模式控制寄存器并赋值初始值counter_value
static void frequency_set(uint8_t counter_port,
                          uint8_t counter_no,
                          uint8_t rwl,
                          uint8_t counter_mode,
                          uint16_t counter_value) {
    // 往控制字寄存器端口0x43写入控制字
    outb(PIT_COUNTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));

    // 再写入counter_value的低八位
    outb(counter_port, (uint8_t)counter_value);

    // 写入高八位
    outb(counter_port, (uint8_t)counter_value >> 8);

}

// 时钟滴答的中断处理函数
static void intr_timer_handler(void) {

    struct task_struct* cur_thread = running_thread();

    ASSERT(cur_thread->stack_magic == 0x20050102);              // 检查栈时候溢出

    cur_thread->elapsed_ticks ++;                               // 记录此线程占用cpu的时间
    ticks ++;                                                   // 从内核第一此处理时间中断后开始至今的滴答数 内核和用户态总共的滴答数

    if (cur_thread->ticks == 0) {

        schedule();                                             // 若时间片用完 则开启调度新的进程上cpu

    } else {

        cur_thread->ticks --;                                   // 当前进程的时间片-1

    }

}

// 初始化PIT8253
void timer_init(void) {

    put_str("timer_init start\n");
    
    // 设置8253的定时周期 也就是中断的周期
    frequency_set(COUNTER0_PORT,
                  COUNTER0_NO,
                  READ_WRITE_LATCH,
                  COUNTER_MODE,
                  COUNTER0_VALUE);

    register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");

}