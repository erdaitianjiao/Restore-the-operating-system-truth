#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

int main() {

    put_str("hello kernel!\n");
    init_all();
    
    ASSERT(1 == 2);
    while(1);
    return 0;

}
