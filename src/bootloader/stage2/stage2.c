#include <stdint.h>
#include "stdio.h"
#include "x86.h"

/*----------------------------------------------------------------------------*/
void puts_realmode(const char* str)
{
    while (*str) {
        x86_realmode_putc(*str);
        ++str;
    }
}

/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    //clrscr();
    printf("Entering stage2 %d\n");
    puts_realmode("Hello from real mode");    
    for (;;)
        ;
}
