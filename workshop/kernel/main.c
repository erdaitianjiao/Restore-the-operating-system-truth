#include "init.h"
#include "print.h"
#include "interrupt.h"

int main() {

    put_str("hello kernel!\n");
    idt_init();
    asm volatile("sti");
    while(1);
    return 0;
    
}
