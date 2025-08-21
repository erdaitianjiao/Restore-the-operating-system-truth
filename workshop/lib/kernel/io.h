/*      机器模式
    b -- 输出寄存器QImode名称 寄存器的低八位 [a - d]
    w -- 寄存器HImode名称     寄存器两个字节的部分

    HImode
        "Half-Integer"模式 表示一个两个字节的整数
    QImode
        "Quarter-Integer"模式 表示一个一个字节的整数

*/

#ifndef __LIB_IO_H
#define __LIB_IO_H

#include "stdint.h"

// 向端口port写入一个字节
static inline void outb(uint16_t port, uint8_t data) {

    // 对端口指定N表示0-255 d表示dx存储端号
    // %b0 表示对应al %wl表示对应dx
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));

}

// 将addr处起始的word_cnt个字写入端口port
static inline void outsw(uint16_t port, const void *addr, uint32_t word_cnt) {

    // +表示此限制既做输出 又做输入
    // outsw是把ds:esi处的16位的内容写入port端口 我们在设置段描述符的时候
    // 已经将 ds es cs 段的选择子赋相同值了 不用担心数据错乱

    asm volatile ("cld\n\t" "rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

// 从port端口读一个字节返回
static inline uint8_t inb(uint16_t port) {

    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));

    return data;

}

// 从端口port读入的word_cnt个字写入addr
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {

    // insw是从port读取16位内容写入es:edi只想的内存

    asm volatile ("cld\n\t" "rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");

}

#endif
