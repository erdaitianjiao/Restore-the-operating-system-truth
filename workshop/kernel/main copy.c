#include "init.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"
#include "stdio.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;
int prog_a_pid = 0, prog_b_pid = 0;


int main(void) {

    put_str("I am kernel\n");
    init_all();

    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");

    intr_enable();                                          // 打开中断 使时钟中断起作用

    console_put_str(" main_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    thread_start("k_thread_a", 31, k_thread_a, "argA ");
    thread_start("k_thread_b", 31, k_thread_b, "argB ");

    while (1);

    return 0;

}

// 在线程中运行的函数
void k_thread_a(void* arg) { 

    char* para = arg;

    console_put_str(" thread_a_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    while(1);

}

// 在线程中运行的函数
void k_thread_b(void* arg) { 

    char* para = arg;

    console_put_str(" thread_b_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    while(1);

}

void u_prog_a(void) {

    char* name = "prog_a";
    printf(" i am %s my pid is:0x%d%c\n", name, getpid(), '\n');
    while(1);
 
}

void u_prog_b(void) {

    char* name = "prog_b";
    printf(" i am %s my pid is:0x%d%c\n", name, getpid(), '\n');
    while(1);
 
}