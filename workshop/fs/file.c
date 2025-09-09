#include "stdint.h"
#include "file.h"
#include "dir.h"
#include "fs.h"
#include "inode.h"
#include "ide.h"
#include "bitmap.h"
#include "super_block.h"

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
    while (local_fd_idx < MAX_FILE_OPEN) {

        if (cur->fd_table[local_fd_idx] == -1) {

            // -1表示free_flot 可用
            cur->fd_table[local_fd_idx] = global_fd_idx;
            break;

        }

        local_fd_idx ++;

    }

    if (local_fd_idx == MAX_FILE_OPEN) {

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
