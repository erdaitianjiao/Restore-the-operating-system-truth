#include "debug.h"
#include "print.h"
#include "interrupt.h"

// 打印文件名 行号 函数名 条件 并使程序悬停
void panic_spin(char* filename,
                int line,
                const char *func,
                const char* condition) {
    intr_disable();

    put_str("\n\n\n ---------error-------- \n");
    put_str("filename:"); put_str(filename); put_str("\n");
    put_str("Line: "); put_int(line); put_char('\n');
    put_str("Func: "); put_str((char*)func); put_char('\n');
    put_str("Condition: "); put_str((char*)condition); put_char('\n');
    
    while (1);
    
    }   