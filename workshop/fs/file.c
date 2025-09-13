#include "stdint.h"
#include "file.h"
#include "dir.h"
#include "fs.h"
#include "inode.h"
#include "ide.h"
#include "bitmap.h"
#include "super_block.h"
#include "string.h"
#include "global.h"
#include "interrupt.h"



struct file file_table[MAX_FILE_OPEN];

// 从文件表file_table[MAX_FILE_OPEN]中读取一个空闲位 成功返回下表 否则返回-1
int32_t get_free_slot_in_global(void) {

    uint32_t fd_idx = 3;
    while (fd_idx < MAX_FILE_OPEN) {

        if (file_table[fd_idx].fd_inode == NULL) {

            break;

        }

        fd_idx ++;

    }

    if (fd_idx == MAX_FILE_OPEN) {

        printk("exceed max open files\n");
        return -1;

    }

    return fd_idx;

}

// 将全局描述符下表安装到进程或者线程自己的文件描述符fd_table中
int32_t pcb_fd_install(int32_t global_fd_idx) {

    struct task_struct* cur = running_thread();
    // 跳过stdin stdout stderr
    uint8_t local_fd_idx = 3;
    while (local_fd_idx < MAX_FILES_OPEN_PER_PROC) {

        if (cur->fd_table[local_fd_idx] == -1) {

            // -1表示free_slot 可用
            cur->fd_table[local_fd_idx] = global_fd_idx;
            break;

        }

        local_fd_idx ++;

    }

    if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {

        printk("exceed max open files_per_proc\n");
        return -1;

    }

    return local_fd_idx;

}

// 分配一个i节点 返回i节点节点号
int32_t inode_bitmap_alloc(struct partition* part) {

    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1 ) {

        return -1;

    }
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;

}

// 分配一个扇区 返回扇区地址
int32_t block_bitmap_alloc(struct partition* part) {

    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1 ){

        return -1;

    }
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    // 和inode_bitmap_malloc不同 此处返回的不是位图索引 而是具体的扇区地址
    return (part->sb->data_start_lba + bit_idx);

}


// 创建文件 若成功则返回文件描述符 否则返回-1
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag) {

    // 后续操作的公共缓冲区
    void* io_buf = sys_malloc(1024);
    if (io_buf == NULL) {

        printk("in file_create: sys_malloc for io_buf failed\n");
        return -1;

    }
    uint8_t rollback_step = 0;              // 用于回滚各资源状态

    // 为新文件分配inode
    int32_t inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1) {

        printk("in file_creat: allocate inode failed\n");
        return -1;

    }
    
    // 此inode要从堆内申请内存 不可生成局部变量 (函数退出会释放)
    // 因为file_table数组文件描述符的inode指针也要指向它
    struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));
    
    if (new_file_inode == NULL) {

        printk("file_create: sys_malloc for inode failed\n");
        rollback_step = 1;
        goto rollback;

    }
    inode_init(inode_no, new_file_inode);           // 初始化i节点

    // 返回的是file_table下表
    int fd_idx = get_free_slot_in_global();
    if (fd_idx == -1) {

        printk("exceed max open filed\n");
        rollback_step = 2;
        goto rollback;

    }
    
    file_table[fd_idx].fd_inode = new_file_inode;
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    file_table[fd_idx].fd_inode->write_deny = false;

    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));

    create_dir_entry(filename, inode_no, FT_REFULAR, &new_dir_entry);
    // create_dir_entry只是内存操作不出意外 不会返回失败
    
    // 同步内存数据到硬盘 
    // a 在目录parent_dir下安装目录项new_dir_entry 写入硬盘后返回true 否则返回falsie
    if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
        
        printk("sync dir_entry to disk failed\n");
        rollback_step = 3;
        goto rollback;

    }
    memset(io_buf, 0, 1024);
    // b 将父目录i节点的内容同步到硬盘
    inode_sync(cur_part, parent_dir->inode, io_buf);
    memset(io_buf, 0, 1024);

    // c 将创建文件的i节点内容同步到硬盘
    inode_sync(cur_part, new_file_inode, io_buf);
    
    // d 将inode_bitmap位图同步到硬盘
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    // e 将创建的文件i节点添加到open_inodes链表
    list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
    new_file_inode->i_open_cnts = 1;

    sys_free(io_buf);
    return pcb_fd_install(fd_idx);

    // 创建文件需要创建多个资源 若创建失败则会执行到下面的回滚步骤
rollback:
    switch (rollback_step) {

        case 3:
            // 失败时 将faile_table中的内容清空
            memset(&file_table[fd_idx], 0, sizeof(struct file));
        case 2:
            sys_free(new_file_inode);
        case 1:
            // 新文件创建的i节点失败 位图中分配的inode_no也要恢复
            bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
            break;
    }
    sys_free(io_buf);
    return -1;


}

// 打开标号为inode_no的inode对应的文件 成功返回文件描述符 失败返回-1
int32_t file_open(uint32_t inode_no, uint8_t flag) {

    int fd_idx = get_free_slot_in_global();
    if (fd_idx == -1) {

        printk("exceed max open files\n");
        return -1;

    }
    file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
    file_table[fd_idx].fd_pos = 0;

    // 每次打开文件 要将fd_pos换原为0 即让文件内的指针指向开头
    file_table[fd_idx].fd_flag = flag;
    bool* write_deny = &file_table[fd_idx].fd_inode->write_deny;

    if (flag & O_WRONLY || flag & O_RDWD) {

        // 只要是关于写文件 判断是否其他进程正在写文件
        // 若是写文件 不考虑 write_deny
        // 以下进入临界区前先关中断
        enum intr_status old_status = intr_disable();
        if (!(*write_deny)) {

            // 如果没有其他进程写该文件 则占用
            *write_deny = true;                 // 置为true 避免多个进程同时写此文件
            intr_set_status(old_status); 

        } else {

            // 直接失败返回
            intr_set_status(old_status);
            printk("file cant be write now, try again later");
            return -1;

        }

    }
    // 若是读文件或者创建文件 则保持默认
    return pcb_fd_install(fd_idx);

}

// 关闭文件
int32_t file_close(struct file* file) {

    if (file == NULL) {

        return -1;

    }
    file->fd_inode->write_deny = false;
    inode_close(file->fd_inode);
    file->fd_inode = NULL;                  // 使文件结构可用
    return 0;

}

// 将内存中的bitmap第bit_dix位所在的512字节同步到硬盘
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp) {

    // 本i节点索引相对于位图扇区的偏移地址
    uint32_t off_sec = bit_idx / 4096;
    // 本i节点相对与位图的字节偏移量
    uint32_t off_size = off_sec * BLOCK_SIZE;

    uint32_t sec_lba;
    uint8_t* bitmap_off;

    // 需要被同步到硬盘的位图只有inode_bitmap和block_bitmap
    switch (btmp) {

        case INODE_BITMAP:
            sec_lba = part->sb->inode_bitmap_lba + off_sec;
            bitmap_off = part->block_bitmap.bits + off_sec;
            break;
        case BLOCK_BITMAP:
            sec_lba = part->sb->block_bitmap_lba + off_sec;
            bitmap_off = part->block_bitmap.bits + off_sec;
            break;

    }

    ide_write(part->my_disk, sec_lba, bitmap_off, 1);

}
