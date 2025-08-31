#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"
#include "list.h"

// 内存标记 用于判断哪个内存池
enum pool_flags {

    PF_KERNEL = 1,                      // 内核内存池
    PF_USER = 2                         // 用户内存池 

};

#define  PG_P_1   1                     // 页表项或页目录项存在属性位
#define  PG_P_0   0                     // 页表项或页目录项存在属性位
#define  PG_RW_R  0                     // R/W 属性位值  读/执行
#define  PG_RW_W  2                     // R/W 属性位值  读/写/执行
#define  PG_US_S  0                     // U/S 属性位值  系统级
#define  PG_US_U  4                     // U/S 属性位值  用户级

#define  DESC_CNT 7                     // 内存描述符的个数


// 虚拟内存池 用于虚拟地址管理
struct virtual_addr {

    struct bitmap vaddr_bitmap;         // 虚拟地址用到的位图结构
    uint32_t vaddr_start;               // 虚拟地址的起始位置

};

// 内存块
struct mem_block {

    struct list_elem free_elem;

};

struct mem_block_desc {

    uint32_t block_size;                // 内存块大小
    uint32_t blocks_per_arena;          // 本arena中可容纳此mem_block的数量
    struct list free_list;              // 目前可用的mem_block链表

};

extern struct pool kernel_pool, user_pool;

void mem_init(void);
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
static void* palloc(struct pool* m_pool);
static void page_table_add(void* _vaddr, void* _page_phyaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void block_desc_init(struct mem_block_desc* desc_arry);
static struct arena* block2arena(struct mem_block* b);
static struct mem_block* arena2block(struct arena* a, uint32_t idx);

void* sys_malloc(uint32_t size);

#endif