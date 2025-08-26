#include "init.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

int main(void) {

    put_str("hello kernel!\n");
    init_all();

    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");
    thread_start("k_thread_a", 31, k_thread_a, "Arga "); 
    thread_start("k_thread_b", 31, k_thread_b, "Argb ");

    intr_enable();              // 打开中断 使时钟中断起作用

    while (1) {

    }

    return 0;

}

// 在线程中运行的函数
void k_thread_a(void* arg) { 

    char* para = arg;

    while (1) {

        console_put_str("v_a:0x");
        console_put_int(test_var_a);

    }
}

// 在线程中运行的函数
void k_thread_b(void* arg) { 

    char* para = arg;

    while (1) {
    
        console_put_str("v_b:0x");
        console_put_int(test_var_b);

    }
}

void u_prog_a(void) {

    while (1) {

        test_var_a ++;

    }
 
}

void u_prog_b(void) {

    while (1) {

        test_var_b ++;

    }
 
}