#ifndef _KERNEL_INTERRUPT_H
#define _KERNEL_INTERRUPT_H

typedef void* intr_handler;
void idt_init(void);

#endif
