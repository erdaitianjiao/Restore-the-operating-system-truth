#include "init.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"

void k_thread_a(void*);
void k_thread_b(void*);

int main(void) {

    put_str("hello kernel!\n");
    init_all();

    thread_start("k_thread_a", 31, k_thread_a, " A_"); 
    thread_start("k_thread_a", 8, k_thread_b, " B_");

    intr_enable();              // 打开中断 使时钟中断起作用

    while (1) {

    }

    return 0;

}

// 在线程中运行的函数
void k_thread_a(void* arg) { 

    // 用void*来通用表示参数 被调用的函数知道自己需要什么类型的参数 自己转换再用 
    char* para = arg;

    while (1) {

        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf)) {

            console_put_str(para); 
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);

        }
        
        intr_set_status(old_status);
    }
}

// 在线程中运行的函数
void k_thread_b(void* arg) { 

    // 用void*来通用表示参数 被调用的函数知道自己需要什么类型的参数 自己转换再用 
    char* para = arg;

    while (1) {
        
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf)) {

            console_put_str(para); 
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);

        }
        
        intr_set_status(old_status);

    }
}