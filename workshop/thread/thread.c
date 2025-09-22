#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "process.h"
#include "sync.h"
#include "syscall.h"

#define PG_SIZE 4096

struct task_struct* main_thread;            // 主进程PCB
struct task_struct* idle_thread;            // idle线程
struct list thread_ready_list;              // 就绪队列
struct list thread_all_list;                // 所有任务队列
static struct list_elem* thread_tag;        // 用于保存队列中的线程节点

struct lock pid_lock;                       // 分配pid锁

extern void switch_to(struct task_struct* cur, struct task_struct* next);
extern void init(void);

// 系统空闲的时候运行的线程
static void idle(void* arg UNUSED) {

    while (1) {

        thread_block(TASK_BLOCKED);

        // 执行hlt时必须要保证目前处于开中断的情况下
        asm volatile ("sti\n\t" "hlt" : : : "memory");

    }

}

// 获取当前线程pcb指针
struct task_struct* running_thread(void) {

    uint32_t esp;

    asm ("mov %%esp, %0" : "=g" (esp));

    // 取esp整数部分 即pcb起始地址
    return (struct task_struct*)(esp & 0xfffff000);

}

// 分配pid
static pid_t allocate_pid(void) {

    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid ++;
    lock_release(&pid_lock);
    return next_pid;

}

// 由kernel_thread去执行function(func_arg)
static void kernel_thread(thread_func* function, void* func_arg) {

    // 执行function前要开中断 避免后面的时钟中断被屏蔽 而无法调度其他线程
    intr_enable();
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
    pthread->pid = allocate_pid();
    strcpy(pthread->name, name);

    if (pthread == main_thread) {

        // 由于把main函数也封装一个线程 所以直接设为TASK_RUNNING
        pthread->status = TASK_RUNNING;

    } else {

        pthread->status = TASK_READY;

    }

    // self_kstack是线程自己在内核态的时候使用的栈顶地址
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);        // 刚开始的位置是最低位置 栈顶位置 + 一页
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;

    // 预留标准输入输出
    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;

    uint8_t fd_idx = 3;
    while (fd_idx < MAX_FILES_OPEN_PER_PROC) {

        pthread->fd_table[fd_idx] = -1;
        fd_idx ++;

    }


    // pthread->stack_magic = 0x19870916; 
    pthread->cwd_inode_nr = 0;
    pthread->parent_pid = -1;                                  
    pthread->stack_magic = 0x20050102;                                      // 自定义的魔数

}

// 创建一个优先级是prio的线程 线程名叫name 执行的函数是function(func_arg)
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {

    // pcb都位于内核空间 包括用户进程pcb也是在内核空间
    struct task_struct* thread = get_kernel_pages(1);
    
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    // 确保之前不在队列中
    ASSERT(!(elem_find(&thread_ready_list, &thread->general_tag)));
    // 加入就绪线程队列
    list_append(&thread_ready_list, &thread->general_tag);

    // 确保之前不在队列中 
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    // 加入全部线程队列
    list_append(&thread_all_list, &thread->all_list_tag);
                  
    return thread;
}

// 将kernel中的main函数完善成主进程
static void make_main_thread(void) {

    // 因为main线程早已在运行
    // 就在loader.S中进入内核时mov esp, 0xc009f000
    // 就是为其预留pcb的 因此pcb的地址是0xc009e000
    // 不需要通过get_kernel_page另分配一页
    main_thread = running_thread();

    // debug
    init_thread(main_thread, "main", 31);


    // main函数是当前进程 当前线程不在thread_ready_list中
    // 所以只能将其加入thread_all_list中
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);

}

// 实现任务调度
void schedule(void) {

    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur = running_thread();
    if (cur->status == TASK_RUNNING) {

        // 若此线程只是cpu时间片到了 将其加入到就绪队列队尾
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);

        // 重新将线程中的ticks重置为其priority
        cur->ticks = cur->priority;
        cur->status = TASK_READY;

    } else {

        // 若此线程需要某件时间发生后才能继续到cpu中运行 不需要加入队列 因为不在就绪队列中

    }

    // 如果就绪队列中没有可运行的任务 就唤醒idle
    if (list_empty(&thread_ready_list)) {

        thread_unblock(idle_thread);

    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;                          // thread_tag 清空

    // 将thread_ready_list中的第一个就绪线程弹出 准备将其调度上cpu
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;

    // 激活页表任务
    process_activate(next);

    switch_to(cur, next);

}

// 当前线程将自己阻塞 标志状态为stat
void thread_block(enum task_status stat) {

    // stat取值为TASK_BLOCKED TASK_WAITTING  TASK_HANGING这几个状态表示不会被调度
    ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));

    enum intr_status old_status = intr_disable();

    struct task_struct* cur_thread = running_thread();
    
    cur_thread->status = stat;                                  // 置状态为stat

    schedule();                                                 // 将当前进程换下处理器

    // 等待当前进程被解除阻塞时才能继续运行下面的intr_set_status
    intr_set_status(old_status);

}

// 将线程解除阻塞
void thread_unblock(struct task_struct* pthread) {
    
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));

    if (pthread->status != TASK_READY) {

        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag)) {

            PANIC("thread_unblock: block thread is in ready_list\n");

        }

        list_push(&thread_ready_list, &pthread->general_tag);               // 放到队列最前头 使其尽快得到调度
        pthread->status = TASK_READY;

    }

    intr_set_status(old_status);

}

// 主动让出cpu 换其他线程运行
void thread_yield(void) {

    struct task_struct* cur = running_thread();
    enum intr_status old_status = intr_disable();

    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);

    cur->status = TASK_READY;
    schedule();

    intr_set_status(old_status);

}

// 获取新的pid
pid_t fork_pid(void) {

    return allocate_pid();

}

// 初始化线程环境
void thread_init(void) {

    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    
    // 创建第一个用户进程
    process_execute(init, "init");
    // 放在第一个初始化 这是第一个进程 init进程pid为1

    // 将当前main函数创建为线程
    make_main_thread();
    
    // 创建idle线程
    idle_thread = thread_start("idle", 10, idle, NULL);

    put_str("thread_init done\n");

}