#include "stdint.h"
#include "stdio.h"


/*----------------------------------------------------------------------------*/
void _cdecl cstart_(uint16_t bootdrive)
{
    printf("formatted text: %c %% %s\r\n", 'a', "string");
    printf("formatted numbers: %d %i %x %p %o %hd %hi %hhu %hhd\r\n", 1234, -5678, 0xdead, 0xbeef, 012345,  (short)27, (short)-42, (unsigned char)20, (char)-10);
    printf("formatted numbers: %ld %lx %lld %llx\r\n", -123456789l, 0xdeadbeeful, 10200300400ll,  0xdeadbeefbaadcafeull);
    for (;;)
        ;
}
