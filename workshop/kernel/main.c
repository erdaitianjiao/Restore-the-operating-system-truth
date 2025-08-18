#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"

int main(void) {

    put_str("hello kernel!\n");
    init_all();

    ASSERT(strcmp("bbb","bbb"));
    while (1);
    return 0;

}
