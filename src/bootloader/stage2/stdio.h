#pragma once
#include <stdint.h>

#define COLOR_BLK 0
#define COLOR_BLU 1
#define COLOR_GRN 2
#define COLOR_CYN 3
#define COLOR_RED 4
#define COLOR_PUR 5
#define COLOR_BRN 6
#define COLOR_GRY 7

void clrscr();
void putc(char c);
void putc_color(char c, uint8_t color);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
