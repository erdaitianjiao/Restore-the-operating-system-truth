#include "console.h"
#include "stdio.h"
#include "stdint.h"
#include "syscall.h"
#include "debug.h"
#include "fs.h"
#include "file.h"
#include "shell.h"
#include "string.h"

#define cmd_len 128             // 最大支持128个字符的命令行输入
#define MAX_ARG_NR 16           // 加上命令外 最多支持15个参数

// 存储输入的指令
static char cmd_line[cmd_len] = {0};

// 用来记录当前目录 是当前目录的缓存 每次执行cd会更新此内容
char cwd_cache[64] = {0};

// 输出提示符
void print_prompt(void) {

    printf("[tianjiao@localhost %s]$ ", cwd_cache);

}

// 从键盘缓冲区中最多读入count个字节到buf
static void readline(char* buf, int32_t count) {

    ASSERT(buf != NULL && count > 0);
    char* pos = buf;
    while (read(stdin_no, pos, 1) != -1 && (pos - buf) < count) {

        // 在不出错情况下 知道找到回车符才返回
        switch (*pos) {

            case '\n':
            case '\r':
                *pos = 0;           // cmd_line的终止字符
                putchar('\n');
                return;
            
            case '\b':
                if (buf[0] != '\b') {

                    -- pos;
                    putchar('\b');

                }
            break;

            // 非控制字符
            default:
                putchar(*pos);
                pos ++;

        }

    }
    printf("readline: can't find entry_key in the cmd_line, max num of char is 128\n");

}

// 简单的shell
void my_shell(void) {

    cwd_cache[0] = '/';
    while (1) {

        print_prompt();
        memset(cmd_line, 0, cmd_len);
        readline(cmd_line, cmd_len);
        if (cmd_line[0] == 0) {

            continue;

        }

    }
    panic("my_shell: should not be here");

}