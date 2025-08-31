#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include "stdint.h"

typedef char* va_list;

uint32_t printf(const char* str, ...);
uint32_t vsprintf(char* str, const char* format, va_list ap);

// debug
void my_print_test(char* str);

#endif
