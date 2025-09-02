#include "memory.h"
#include "stdio-kernel.h"
#include "ide.h"
#include "debug.h"
#include "global.h"
#include "ioqueue.h"
#include "io.h"

// 定义硬盘各寄存器的端口号
#define reg_data(channel)           (channel->port_base + 0)
#define reg_error(channel)          (channel->port_base + 1)
#define reg_sect_cnt(channel)       (channel->port_base + 2)
#define reg_lba_l(channel)          (channel->port_base + 3)
#define reg_lba_m(channel)          (channel->port_base + 4)
#define reg_lba_h(channel)          (channel->port_base + 5)
#define reg_dev(channel)            (channel->port_base + 6)
#define reg_status(channel)         (channel->port_base + 7)
#define reg_cmd(channel)            (reg_status(channel))
#define reg_alt_status(channel)     (channel->port_base + 0x206)
#define reg_atl(channel)            (reg_alt_status(channel))


// reg_status 寄存器的一些关键位
#define BIT_STAT_BUSY               0x80        // 硬盘忙
#define BIT_STAT_DRDY               0x40        // 驱动器准备还
#define BIT_STAT_DRQ                0x8         // 数据传输准备好了

// device 寄存器的一些关键位
#define BIT_DEV_MBS                 0xa         // 第七位和第五位固定为1
#define BIT_DEV_LBA                 0x40
#define BIT_DEV_DEV                 0x10

// 一些键盘操作指令
#define CMD_IDENTIFY                0xec        // identify指令
#define CMD_READ_SECTOR             0x20        // 读扇区指令
#define CMD_WRITE_SECTOR            0x30        // 写扇区指令

// 定义可读写的最大扇区数
#define max_lba ((80 * 1024 * 1024 / 512) - 1)  // 只支持80mb磁盘

uint8_t channel_cnt;                            // 按磁盘数计算的通道数
struct ide_channel channels[2];                 // 有两个ide通道

// 选择读写磁盘
static void select_disk(struct disk* hd) {

    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;

    if (hd->dev_no == 1) {

        // 如果是从盘就置成DEV位为1
        reg_device |= BIT_DEV_DEV;

    }

    outb(reg_dev(hd->my_channel), reg_device);

}

// 向硬盘控制器写入起始扇区地址以及要读写的扇区数
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {

    ASSERT(lba <= max_lba);
    struct ide_channel* channel = hd->my_channel;

    // 写入要读写的扇区数
    outb(reg_sect_cnt(channel), sec_cnt);

    // 写入lba地址 即扇区号
    outb(reg_lba_l(channel), lba);

    // lba地址的第八位 不用单独取出低八位
    // outb函数中的汇编指令outb %b0, %w1 会只用al

    outb(reg_lba_m(channel), lba >> 8);             // lba地址的 8-15 位
    outb(reg_lba_h(channel), lba >> 16);            // lba地址的 16-23位

    // 因为lba地址的第24位到27位要存储在device寄存器的0-3位
    // 无法单独写入4位 所以在此把device寄存器再重新写入一次
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no = 1 ? BIT_DEV_DEV : 0) | lba >> 24);

}

// 向通道channel发命令cmd
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {

    // 只要向硬盘发出了命令便将此标记置为true 硬盘中断处理程序需要根据他来判断 
    channel->expecting_intr = true;
    
    outb(reg_cmd(channel), cmd);


}

// 硬盘读入sec_cnt个扇区的数据到buf
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {

    uint32_t size_in_byte;

    if (sec_cnt == 0) {

        // 因为sec_cnt是8位变量 由主函数为其赋值时 若为256则会将做高位的1丢掉变成0
        size_in_byte = 256 * 512;

    } else {

        size_in_byte = sec_cnt * 512;

    }

    insw(reg_data(hd->my_channel), buf, size_in_byte / 2);

}

// 将buf中sec_cnt扇区的数据写入硬盘
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {

    uint32_t size_in_byte;

    if (sec_cnt == 0) {

        size_in_byte = 256 * 512;

    } else {

        size_in_byte = sec_cnt * 512;

    }

    outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);

}

static bool busy_wait(struct disk* hd) {

    struct ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;                // 可以等30000ms

    while (time_limit -= 10 >= 0) {

        if (!(inb(reg_status(channel)) & BIT_STAT_BUSY)) {

            return (inb(reg_status(channel) & BIT_STAT_DRQ));

        } else {

            mtime_sleep(10);                        // 睡眠10ms

        }

    }
    
    return false;

}

// 从硬盘中读取sec_cnt个扇区到buf
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {

    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);

    lock_acquire(&hd->my_channel);

    // 1 先选择操作磁盘的硬盘
    select_disk(hd);

    uint32_t secs_op;                   // 每次操作的扇区数
    uint32_t secs_done = 0;             // 已完成的扇区数

    while (secs_done < sec_cnt) {

        if (secs_done + 256 < sec_cnt) {

            secs_op = 256;

        } else {

            secs_op = sec_cnt - secs_done;

        }
 
    

        // 2 写入待读入的扇区数和起始扇区号
        select_sector(hd, lba + secs_done, secs_op);

        // 3 执行的命令写入reg_cmd寄存器
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);

        // 4 检测硬盘状态是否可读
        if (!busy_wait(hd)) {

            // 若失败
            char error[64];
            sprintf(error, "%s write sector %d failed!\n", hd->name, lba);
            PANIC(error);

        }

        // 5 将数据写入硬盘
        write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);

        // 在硬盘响应期间阻塞自己
        sema_down(&hd->my_channel->disk_done);
        secs_done += secs_op;
    
    }

    // 醒来后开始释放锁
    lock_release(&hd->my_channel->lock);

}

// 硬盘中断处理程序
void intr_hd_handler(uint8_t irq_no) {

    ASSERT(irq_no == 0x2e || irq_no == 0x2f);

    uint8_t ch_no = irq_no - 0x2e;
    struct ide_channel* channel = &channels[ch_no];

    ASSERT(channel->irq_no == irq_no);
    // 不必担心此中断是否对应的是这一次的expecting_intr
    // 每次读写磁盘时会申请锁 从而保证了同步一致性
    if (channel->expecting_intr) {

        channel->expecting_intr = false;
        sema_up(&channel->disk_done);

        // 读取状态寄存器使硬盘控制器认为此次的中断已经被处理从而硬盘可以继续执行新的读写
        inb(reg_status(channel));

    }

}


// 磁盘数据结构初始化
void ide_init() {

    printk("ide_init start\n");

    uint8_t hd_cnt = *((uint8_t*)(0x475));      // 获取硬盘数量

    ASSERT(hd_cnt > 0);
    // 一个ide通道上有两个硬盘 根据硬盘数量反推有几个ide通道
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

    struct ide_channel* channel;
    uint8_t channel_no = 0;
    
    // 处理每个通道上的硬盘
    while (channel_no < channel_cnt) {

        channel = &channels[channel_no];
        sprintf(channel, "ide%d", channel_no);

        // 为每个ide通道初始化端口基址及中断向量
        switch (channel_no) {

            case 0:

                // ide0通道的起始端口号是0x1f0
                channel->port_base = 0x1f0;
                
                // 从片8259a上倒数第二的中断引脚 
                // 硬盘 也就是ide0通道的中断向量号
                channel->irq_no = 0x20 + 14;
                break;

            case 1:

                // ide1通道的起始端口号是0x170
                channel->port_base = 0x170;

                // 从片8259a上倒数第一的中断引脚
                // 用来相应ide1通道的硬盘中断
                break;

        }

        // 未向硬盘写入指令的时候不期待硬盘的中断
        channel->expecting_intr = false;

        lock_init(&channel->lock);

        // 初始化为0 目的是向硬盘控制器请求数据后 硬盘驱动sema_down此信号会阻塞线程
        // 直到硬盘完成后通过发中断
        // 由中断处理程序将此信号量sema_up 唤醒线程
        sema_init(&channel->disk_done, 0);
        channel_no ++;                          // 下一个channel;

    }

    while (channel_no < channel_cnt) {
        
        // 处理每个通道上的磁盘
        register_handler(channel->irq_no, intr_hd_handler);
        channel_no ++;

    }

    printk("ide init done\n");

}