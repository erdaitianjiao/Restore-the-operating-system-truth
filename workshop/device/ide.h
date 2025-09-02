#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "sync.h"
#include "bitmap.h"
#include "list.h"

// 分区结构
struct partition {

    uint32_t start_lba;                 // 起始扇区
    uint32_t sec_cnt;                   // 扇区数量
    struct disk* my_disk;               // 分区所属硬盘
    struct list_elem part_tag;          // 用于对队列中的标记
    char name[8];                       // 分区名称
    struct super_block* sb;             // 本分区的超级块
    struct bitmap block_bitmap;         // 块位图
    struct bitmap inode_bitmap;         // i 节点位图
    struct list open_incode;            // 本分区打开的 i 节点队列

};

// 硬盘结构
struct disk {

    char name[8];                       // 本硬盘的名称
    struct ide_channel* my_channel;     // 此块硬盘归属于哪个ide通道
    uint8_t dev_no;                     // 本磁盘是主0 还是从1
    struct partition prim_parts[4];     // 主分区顶多是4个
    struct partition logic_parts[8];    // 逻辑分区数量无限 支持的上限是4

};

// ata通道结构
struct ide_channel {

    char name[8];                       // 本ata通道名称
    uint16_t port_base;                 // 本通道的起始端口号
    uint8_t irq_no;                     // 本通道所用的终端号
    struct lock lock;                   // 通道锁
    bool expecting_intr;                // 表示等待硬盘中断
    struct semaphore disk_done;         // 用于阻塞 唤醒驱动程序
    struct disk devices[2];             // 一个通道两个磁盘 一个主 一个从

};
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt);
static bool busy_wait(struct disk* hd);
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void intr_hd_handler(uint8_t irq_no);
void ide_init();

#endif