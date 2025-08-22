#include "sync.h"
#include "interrupt.h"
#include "debug.h"

// 初始化信号量
void sema_init(struct semaphore* psema, uint8_t value) {

    psema->value = value;               // 为信号量赋予初值
    list_init(&psema->waiters);         // 初始化信号量等待队列

}

// 初始化锁
void lock_init(struct lock* plock) {

    plock->holder = NULL;
    plock->holder_repeat_nr = 0;

    sema_init(&plock->semaphore, 1);    // 信号量初始化为1

}

// 信号量down操作
void sema_down(struct semaphore* psema) {

    // 关中断保证原子操作
    enum intr_status old_status = intr_disable();

    while (psema->value == 0) {
    // 若value为0表示被别人持有

        // 当前进程不应该在已知信号量的waiter队列中
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));

        if (elem_find(&psema->waiters, &running_thread()->general_tag)) {

            PANIC("sema_down: thread blocked has been in waiter_list\n");

        }

        // 若信号量的值等于0 则当前进程把自己加入等待队列 然后阻塞自己
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);                                     // 线程阻塞 直到被唤醒

    }

    // 若value等于1或者被唤醒后 会执行下面的代码 也就是获得了锁
    psema->value --;
    ASSERT(psema->value == 0);

    // 恢复之前的状态
    intr_set_status(old_status);

}

// 信号量原子操作
void sema_up(struct semaphore* psema) {

    // 关中断 保证原子操作
    enum intr_status old_status = intr_disable();
    
    ASSERT(psema->value == 0);

    if (!list_empty(&psema->waiters)) {

        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);

    }

    psema->value ++;
    ASSERT(psema->value == 1);
    
    // 恢复之前的状态
    intr_set_status(old_status);

}

// 获得锁lock
void lock_acquire(struct lock* plock) {

    // 排除曾经自己 已经持有锁但是还未将其释放的情况
    if (plock->holder != running_thread()) {

        sema_down(&plock->semaphore);           // 对信号量P操作 原子操作
        plock->holder = running_thread();

        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;

    } else {

        plock->holder_repeat_nr ++;

    }

}

// 释放锁plock
void lock_release(struct lock* plock) {

    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1) {

        plock->holder_repeat_nr --;
        return;

    }

    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL;                   // 把锁的持有者置空放在V操作之前
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);             // 信号量的V操作 是原子操作

}