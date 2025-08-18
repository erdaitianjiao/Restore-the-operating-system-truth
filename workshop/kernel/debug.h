#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

void panic_spin(char* filename, int line, const char* func, const char* condition);
/*
    __VA_AGES__
    代表所有与省略号相对应的参数
    ... 代表定义的宏其参数可变

*/
#define PANIC(...) panic_spin (__FILE__, __LINE__, __func__, __VA_ARGS__)


#ifdef NDBUG
    #define  ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION)                           \
if (CONDITION) {} else {                            \
/* 符号#让编译器将宏的参数转化为字符串字面量 */         \
    PANIC(#CONDITION);                               \
}
#endif

#endif