#include "init.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"
#include "stdio.h"
#include "memory.h"
#include "syscall.h"
#include "file.h"
#include "fs.h"
#include "string.h"
#include "debug.h"

void init(void);

int main(void) {

    put_str("I am kernel\n");
    init_all();
    cls_screen();
    console_put_str("[tianjiao@localhost /]$ ");
    ps();
    while (1);
    return 0;

}

void init(void) {

    uint32_t ret_pid = fork();
    if (ret_pid) {

        while (1);

    } else {

        my_shell();

    }
    PANIC("init: should not be here");

}