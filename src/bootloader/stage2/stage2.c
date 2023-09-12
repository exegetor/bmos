#include <stdint.h>
#include "stdio.h"

/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clrscr();
    printf(" Entering stage2\r\n");
}
