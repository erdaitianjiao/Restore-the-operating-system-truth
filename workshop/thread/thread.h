#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"

// 自定义通用函数类型 将成为很多线程函数中作为形式参数
typedef void thread_func(void*);

// 进程或线程状态
enum task_status {

    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED

};

// 中断栈 intr_stack
// 此结构用于发生中断时保护程序(进程或线程)的上下文结构
// 进程或者线程被外部中断打断时 会按照以下结构压入上下文
// 寄存器 intr_exit 中的出栈操作是此结构体的逆操作
// 此栈在线程自己的内核栈中位置固定 所在页的最顶端
struct intr_stack {

    uint32_t vec_no;            // kernel.S 宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;         // 虽然pushad把esp也压入 但esp是不断变化的 所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // 以下是cpu从低特权进入高特权时压入的
    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflages;
    void* esp;
    uint32_t ss;

};

// 线程栈 thread_task
// 线程自己的栈 用于存储线程中待执行的函数
// 仅用在switch_to时保存线程环境
// 实际位置取决于实际运行情况
struct thread_stack {

    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;


    // 线程第一次执行时 eip指向待调用的函数kernel_thread
    // 其他时候 eip是指向switch_to的返回值的

    void (*eip)(thread_func* func, void* func_arg);

    // 以下用于第一次被调度上cpu使用
    // 参数unused_ret只为占位置充数为返回地址
    void (*unused_retaddr);             
    thread_func* function;              // 由kernel_thread所调用的函数名
    void* func_arg;                     // 由kernel_thread所调用的函数所需要的参数

};

// 进程或线程的pcb 程序控制块
struct task_struct {

    uint32_t* self_kstack;              // 各内核线程都用自己的内核栈
    enum task_status status;
    uint8_t priority;                   // 线程优先级
    char name[16];
    uint32_t stack_magic;               // 栈的边界标记 用于检测栈溢出

};

static void kernel_thread(thread_func* function,void* func_arg);
void thread_create(struct task_struct* pthread,thread_func function,void* func_arg);
void init_thread(struct task_struct* pthread,char* name,int prio);
struct task_struct* thread_start(char* name,int prio,thread_func function,void* func_arg);

#endif