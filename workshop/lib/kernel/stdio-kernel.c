#include "stdio-kernel.h"
#include "print.h"
#include "stdio.h"
#include "console.h"
#include "global.h"

#define va_start(args, first_fix) args = (va_list)&first_fix
#define va_end(args) args = NULL

// 提供给内核使用的初始化输出函数
void printk(const char* format, ...) {

    va_list args;
    va_start(args, format);
    char buf[1024];
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);

}