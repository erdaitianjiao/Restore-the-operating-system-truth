#ifndef __KERNEL_SHELL_H
#define __KERNEL_SHELL_H

#include "fs.h"
#include "file.h"

extern char final_path[MAX_PATH_LEN];           // 用于洗路径时的缓冲

void print_prompt(void);
void my_shell(void);

#endif

