#include <stdint.h>
#include "stdio.h"

/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clrscr();
    for (int i = 0; i < 1000; i++)
        printf("Entering stage2 %d\n", i);
}
