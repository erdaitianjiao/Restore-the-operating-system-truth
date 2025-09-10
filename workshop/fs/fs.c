#include "fs.h"
#include "stdint.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "string.h"
#include "ide.h"
#include "stdio-kernel.h"
#include "memory.h"
#include "debug.h"
#include "list.h"
#include "stdio-kernel.h"

struct partition* cur_part;                 // 默认情况下操作的是哪个分区

// 在分区链表中找到名为part_name的分区 并赋值给cur_part
static bool mount_partition(struct list_elem* pelem, int arg) {

    char* part_name = (char*)arg;
    struct partition* part = elem2entry(struct partition, part_tag, pelem);

    if (!strcmp(part->name, part_name)) {

        cur_part = part;
        struct disk* hd = cur_part->my_disk;

        // sb_buf用于存储硬盘上读取的超级块
        struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);

        // 在内存中创建分区cur_part的超级块
        cur_part->sb = (struct super_block*)sys_malloc(sizeof(struct super_block));

        if (cur_part->sb == NULL) {

            PANIC("alloc memory failed!");

        }

        // 读入超级块
        memset(sb_buf, 0, SECTOR_SIZE);
        ide_read(hd, cur_part->start_lba + 1, sb_buf, 1);

        // 把sb_buf中超级块的信息复制到分区的超级块sb中
        memcpy(cur_part->sb, sb_buf, sizeof(struct super_block));

        // - 将硬盘上的块位图读入到内存
        cur_part->block_bitmap.bits = (uint8_t*)sys_malloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
        
        if (cur_part->block_bitmap.bits == NULL) {

            PANIC("clloc memory failed!");

        }

        cur_part->block_bitmap.btmp_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;

        // 从硬盘上读入块位图到分区的block_bitmap.bits
        ide_read(hd, sb_buf->block_bitmap_lba, cur_part->block_bitmap.bits, sb_buf->block_bitmap_sects);

        // - 将硬盘上的inode位图读入内存
        cur_part->inode_bitmap.bits = (uint8_t*)sys_malloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);

        if (cur_part->inode_bitmap.bits == NULL) {

            PANIC("alloc memory failed!");

        } 

        cur_part->inode_bitmap.btmp_bytes_len = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
        
        // 从硬盘上读取inode位图到分区的inode_bitmap.bits
        ide_read(hd, sb_buf->inode_bitmap_lba, cur_part->inode_bitmap.bits, sb_buf->inode_bitmap_sects);


        list_init(&cur_part->open_inodes);
        printk("mount %s done!\n", part->name);

        return true;            // 使得list_traversal结束遍历

    }

    return false;               // 使得list_traversal继续遍历

}

// 格式化分区 也就是初始化分区的元信息 创建文件系统
static void partition_format(struct partition* part) {

    // block_bit_map_init 为了实现方便 一个块大小是一个扇区
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;

    // i节点位图所占用的扇区数量 最多支持4096个文件
    uint32_t inode_bitmap_sects = 
    DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);

    uint32_t inode_table_sects = DIV_ROUND_UP((sizeof(struct inode) * MAX_FILES_PER_PART), 
                                               SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + 
                          inode_bitmap_sects + inode_table_sects; 
    uint32_t free_sects = part->sec_cnt - used_sects;

    // - 简单处理块位图占据的扇区数量
    uint32_t block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    
    // block_bitmap_bit_len是位图中位的长度 也是可用块的数量
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

    // - 超级块初始化
    struct super_block sb;
    sb.magic = 0x20050102;
    sb.sec_cnt = part->sec_cnt;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.part_lba_base = part->start_lba;

    // 第0块是引导块 第1块是超级块
    sb.block_bitmap_lba = sb.part_lba_base + 2;
    sb.block_bitmap_sects = block_bitmap_sects;
    
    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;
    
    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);

    printk("%s info:\n", part->name);
    printk("    magic:0x%x\n", sb.magic);
    printk("    part_lba_base:0x%x\n", sb.part_lba_base);
    printk("    all_sectors:0x%x\n", sb.sec_cnt);
    printk("    inode_cnt:0x%x\n", sb.inode_cnt);
    printk("    block_bitmap_lba:0x%x\n", sb.block_bitmap_lba);
    printk("    block_bitmap_sects:0x%x\n", sb.block_bitmap_sects);
    printk("    inode_bitmap_lba:0x%x\n", sb.inode_bitmap_lba);
    printk("    inode_bitmap_sects:0x%x\n", sb.inode_bitmap_sects);
    printk("    inode_table_lba:0x%x\n", sb.inode_table_lba);
    printk("    inode_table_sects:0x%x\n", sb.inode_table_sects);
    printk("    data_start_lba:0x%x\n", sb.data_start_lba);

    struct disk* hd = part->my_disk;

    // 1 将超级快写入本分区的1扇区
    ide_write(hd, part->start_lba + 1, &sb, 1);
    printk("    super_block_iba:0x%x\n", part->start_lba + 1);

    // 找出数量最大的元信息 用其尺寸做存储缓冲区
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? 
                                sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;

    // 申请的内存由内存管理系统清零后返回
    uint8_t* buf = (uint8_t*)sys_malloc(buf_size);

    // 2 将位图初始化写入sb.block_bitmap_lba
    // 第0块预留给根目录 位图中先占位
    buf[0] |= 0x01;                                                
    uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
    uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
    // last_size是位图最后一个扇区中不足一个扇区的部分
    uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);

    // 2.1 先将位图中最后一个字节到其所在的扇区的结束全置为1 即超出的部分直接置为已占用
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);

    // 2.2 再将上一步中覆盖的最后一个字节内的有效位重新置0
    uint8_t bit_idx = 0;
    while (bit_idx <= block_bitmap_last_bit) {

        buf[block_bitmap_last_byte] &= ~(1 << bit_idx ++);

    }

    ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    // 3 将inode位图初始化并写入sb.inode_bitmap_lba
    memset(buf, 0, buf_size);       // 先清空缓冲区
    buf[0] |= 0x1;                  // 第0个inode分配给了根目录
    // inode_table中共4096个inode
    // 位图 inode_bitmap正好占用一个扇区
    // inode_bitmap_sects等于1
    // 所以位图中的位全部代表inode_table中的indoe
    // 无需处理剩余部分
    ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

    // 4 将inode数组初始化并写入sb.inode_table_lba
    // 准备写inode_table中的第0项 即根目录所在的indoe
    memset(buf, 0, buf_size);       // 清空缓冲区buf
    struct inode* i = (struct inode*)buf;
    i->i_size = sb.dir_entry_size * 2;      // .和..
    i->i_no = 0;                            // 根目录占inode数组中的第0位
    i->i_sectors[0] = sb.data_start_lba;    

    // 其他元素都初始化为0
    ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

    // 5 将根目录写入sb.data_start_lba
    // 写入根目录的两个目录项 .和..
    memset(buf, 0, buf_size);
    struct dir_entry* p_de = (struct dir_entry*)buf;

    // 初始化当前目录 "."
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    p_de ++;

    // 初始化当前父目录 ".."
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = 0;                 // 根目录的父目录依然是自己
    p_de->f_type = FT_DIRECTORY; 
    
    // sb.data_start_lba已经分配给了根目录 里面是根目录的目录项
    ide_write(hd, sb.data_start_lba, buf, 1);
    
    printk("    root_dir_lba:0x%x\n", sb.data_start_lba);
    printk("%s format done\n", part->name);

    sys_free(buf);

}

// 将最上层的路径名称解析出来
static char* path_parse(char* pathname, char* name_store) {

    if (pathname[0] = '/') {

        // 根目录不需要单独解析
        // 如果路径出现1个或者多个连续的字符 '/' 将这些跳过 如"///a/b"
        while ((++ pathname) == '/');

    }

    // 开始解析路径
    while (*pathname != '/' && *pathname != 0) {

        *name_store ++ = *pathname ++;

    }

    if (pathname[0] = 0) {

        return NULL;

    }

    return pathname;

}

// 在磁盘上搜索文件系统 若没有则格式化分区创建文件系统
void filesys_init() {

    uint8_t channel_no = 0, dev_no, part_idx = 0;

    // sb_buf 用于存储从硬盘读入的超级块
    struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);

    if (sb_buf == NULL) {

        PANIC("alloc memory failed!");

    }

    printk("searching filesystem...\n");

    while (channel_no < channel_cnt) {

        dev_no = 0;
        while (dev_no < 2) {

            if (dev_no == 0) {
                // 跳过裸盘hd60M.img
                dev_no ++;
                continue;
                
            }

            struct disk* hd = &channels[channel_no].devices[dev_no];
            struct partition* part = hd->prim_parts;
            while (part_idx < 12) {     // 4个主分区加8个逻辑分区
                
                if (part_idx == 4) {
                    
                    // 开始处理逻辑分区
                    part = hd->logic_parts;

                }

                // channel数组是全局变量 默认值为0 disk属于嵌套结构
                // partition又为disk嵌套结构 因此partition中的成员也默认为0
                // 若partition未初始化 则partition中的成员也为0
                if (part->sec_cnt != 0) {       // 如果分区存在

                    memset(sb_buf, 0, SECTOR_SIZE);
                    
                    // 只读出分区的超级块 根据魔数是否正确来判断是否存在文件系统
                    ide_read(hd, part->start_lba + 1, sb_buf, 1);

                    // 只支持自己的文件系统 若磁盘上有文件系统就不再格式化了
                    if (sb_buf->magic == 0x20050102) {

                        printk("%s has filesystem\n", part->name);

                    } else {    // 其余文件系统一律不支持 按无效文件系统处理

                        printk("formatting %s's partition %s...\n", hd->name, part->name);
                        partition_format(part);

                    }

                }

                part_idx ++;
                part ++;            // 下一分区

            }

            dev_no ++;              // 下一磁盘

        }

        channel_no ++;             // 下一通道

    }

    sys_free(sb_buf);

    // 确定默认操作的分区
    char default_part[8] = "sdb1";
    list_traversal(&partition_list, mount_partition, (int)default_part);

}

