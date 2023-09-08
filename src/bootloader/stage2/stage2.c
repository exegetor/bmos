#include "stdint.h"
#include "stdio.h"


/*----------------------------------------------------------------------------*/
void _cdecl cstart_(uint16_t bootdrive)
{
    puts("hello world");
    for (;;)
        ;
}
