#pragma once
#include <stdint.h>

void putc(const char c);
void puts(const char* str);
void printf(char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint16_t count);