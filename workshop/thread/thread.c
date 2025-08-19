#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

// 由kernel_thread去执行function(func_arg)
static void kernel_thread(thread_func* function, void* func_arg) {

    function(func_arg);

}

// 初始化线程栈 thread_stack
// 将待执行的函数和参数放到thread_task中相应的位置
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {

    // 先预留中断使用栈的空间
    pthread->self_kstack -= sizeof(struct intr_stack);
    
    // 再预留出线程栈空间 见thread.h中定义
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = \
    kthread_stack->esi = kthread_stack->edi = 0;

}

// 初始化线程基本信息
void init_thread(struct task_struct* pthread, char* name, int prio) {

    memset(pthread, 0, sizeof(*pthread));
    
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;

    // self_kstack是线程自己在内核态的时候使用的栈顶地址
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);        // 刚开始的位置是最低位置 栈顶位置+一页
    // pthread->stack_magic = 0x19870916;                                   
    pthread->stack_magic = 0x23333333;                                      // 自定义的魔数

}

// 创建一个优先级是prio的线程 线程名叫name 执行的函数是function(func_arg)
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {

    // pcb都位于内核空间 包括用户进程pcb也是在内核空间
    struct task_struct *thread = get_kernel_pages(1);
    
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    asm volatile ("movl %0, %%esp\n\t"
                  "pop %%ebp\n\t"
                  "pop %%ebx\n\t"
                  "pop %%edi\n\t"
                  "pop %%esi\n\t"
                  "ret"
                  : : "g" (thread->self_kstack)
                  : "memory");
    return thread;
}