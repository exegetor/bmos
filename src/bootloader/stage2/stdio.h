#pragma once
#include <stdint.h>

void clrscr();
void putc(const char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint16_t count);