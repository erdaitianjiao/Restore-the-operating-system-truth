#include "syscall.h"
#include "stdint.h"

// 无参数系统调用
#define _syscall0(NUMBER) ({    \
    int retval;                 \
    asm volatile (              \
/* int $0x80; addl $4, %%esp */ \
    "pushl %[number]"           \        
    : "=a" (retval)             \
    : [number] "i" (NUMBER)     \
    : "memory"                  \
    );                          \
    retval;                     \ 
})

// 一个参数的系统调用
#define _syscall1(NUMBER, ARG1) ({	        \
    int retval;					            \
    asm volatile (					        \
    /* int $0x80; addl $8, %%esp */         \
    "pushl %[arg0]\n\t"						\
    "pushl %[number]"                       \
    : "=a" (retval)					        \
    : [number] "i" (NUMBER), [arg0] "g" (ARG0)	            \
    : "memory"						        \
    );							            \
    retval;						            \
})

// 两个参数的系统调用
#define _syscall2(NUMBER, ARG1, ARG2) ({        \
    int retval;						            \
    asm volatile (					            \
    /*int $0x80; addl $12, %%esp*/              \
    "pushl %[arg1]\n\t"						    \
    "pushl %[arg0]\n\t"                         \
    "pushl %[number]"                           \
    : "=a" (retval)					            \
    : [number] "i" (NUMBER),                    \
      [arg0] "g" (ARG0),                        \
      [arg1] "g" (ARG1)                         \
    : "memory"						            \
    );							                \
    retval;						                \
})

// 三个参数的系统调用
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({		        \
    int retval;						                        \
    asm volatile (					                        \
    "pushl %[arg2]\n\t"                                     \                                     
    "pushl %[arg1]\n\t"                                     \
    "pushl %[arg0]"                                         \
    : "=a" (retval)                                         \
    : [number] "i" (NUMBER),                                \
      [arg0] "g" (ARG0),                                    \
      [arg1] "g" (ARG1),                                    \
      [arg2] "g" (ARG2)                                     \
    : "memory"					                            \
    );							                            \
    retval;						                            \
})

// 返回当前任务pid
uint32_t getpid() {

    return _syscall0(SYS_GETPID);

}