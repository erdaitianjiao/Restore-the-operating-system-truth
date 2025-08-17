#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define PIC_M_CTRL      0x20            // 主片控制端口0x20
#define PIC_M_DATA      0x21            // 主片数据端口0x21
#define PIC_S_CTRL      0xa0            // 从片数据端口0xa0
#define PIC_S_DATA      0xa1            // 从片数据端口0xa1

#define IDT_DESC_CNT    0x21

// 中断描述结构体
struct gate_desc {

    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t   dcount;                   // 双字计数字段 门描述的第四字节

    uint8_t attribute;
    uint16_t func_offset_high_word;

};

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];                                      // 中断描述表 其实就是中断门描述数组

extern intr_handler intr_entry_table[IDT_DESC_CNT];                             // 申明引用在kernel.s中的中断处理函数入口数组


// 创建中断门描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {

    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
 
}

// 初始化中断描述符
static void idt_desc_init(void) {

    int i;
    for (i = 0; i < IDT_DESC_CNT; i ++) {

        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);

    }

    put_str("idt_desc_init_done\n");

}

// 完成有关中断的所有初始化工作
void idt_init() {

    put_str("idt_init start\n");
    idt_desc_init();                    // 初始化中断描述符
    pic_init();                         // 初始化8259A
    
    // 加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0" : : "m" (idt_operand));

}

