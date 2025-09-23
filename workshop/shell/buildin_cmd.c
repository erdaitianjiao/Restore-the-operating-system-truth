#include "stdint.h"
#include "string.h"
#include "debug.h"
#include "fs.h"
#include "file.h"
#include "syscall.h"

// 将路径old_abd_path中的..和.转换为实际路径后new_abs_path
static void wash_path(char* old_abs_path, char* new_abs_path) {

    ASSERT(old_abs_path[0] == '/');
    char name[MAX_FILE_NAME_LEN] = {0};
    char* sub_path = old_abs_path;
    sub_path = path_parse(sub_path, name);

    if (name[0] == 0) {

        new_abs_path[0] = '/';
        new_abs_path[1] = 0;
        return;

    }
    // 避免传给new_pas_path的值不干净
    new_abs_path[0] = 0;
    strcat(new_abs_path, "/");
    while (name[0]) {

        // 如果是上一级目录
        if (!strcmp("..", name)) {

            char* slash_ptr = strrchr(new_abs_path, '/');
            // 如果未到new_abs_path中的顶层目录 就将最右边的'/'替换成0
            if (slash_ptr != new_abs_path) {

                *slash_ptr = 0;

            } else {

                *(slash_ptr + 1) = 0;

            }

        } else if (strcmp(".", name)) {

            // 如果路径不是. 就将name 拼接到new_abs_path后面
            if (strcmp(new_abs_path, "/")) {

                strcat(new_abs_path, "/");

            }
            strcat(new_abs_path, name);

        }
        // 继续遍历下一层目录
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (sub_path) {

            sub_path = path_parse(sub_path, name);

        }
    
    }

}

// 将path处理成不含.和..的绝对目录 存储在final_path中
void make_clear_abs_path(char* path, char* final_path) {

    char abs_path[MAX_PATH_LEN] = {0};
    // 先判断是否输入的是绝对路径
    if (path[0] != '/') {

        memset(abs_path, 0, MAX_PATH_LEN);
        if (getcwd(abs_path, MAX_PATH_LEN) != NULL) {

            if (!((abs_path[0] == '/') && (abs_path[1] == 0))) {

                strcat(abs_path, "/");

            }

        }

    }
    strcat(abs_path, path);
    wash_path(abs_path, final_path);

}