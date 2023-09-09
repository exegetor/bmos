#include "stdio.h"
#include "x86.h"

int* printf_number(int *argp, int length, bool sign, int radix);


/*----------------------------------------------------------------------------*/
void putc(char c)
{
    x86_video_write_char_teletype(c, 0);
}

/*----------------------------------------------------------------------------*/
void puts(const char *str)
{
    while (*str) {
        putc(*str);
        str++;
    }
}


/*----------------------------------------------------------------------------*/
#define PRINTF_STATE_NORMAL        0
#define PRINTF_STATE_LENGTH        1
#define PRINTF_STATE_LENGTH_SHORT  2
#define PRINTF_STATE_LENGTH_LONG   3
#define PRINTF_STATE_SPEC          4

#define PRINTF_LENGTH_DEFAULT      0
#define PRINTF_LENGTH_SHORT_SHORT  1
#define PRINTF_LENGTH_SHORT        2
#define PRINTF_LENGTH_LONG         3
#define PRINTF_LENGTH_LONG_LONG    4

void _cdecl printf(const char *fmt, ...)
{
    int *argp = (int*)&fmt;
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;  
    int radix = 10;
    bool sign = false;
    argp++;

    while ( *fmt) {
        switch (state) {
        case PRINTF_STATE_NORMAL:
            switch (*fmt) {
            case '%':   state = PRINTF_STATE_LENGTH;
                        break;
            default:    putc(*fmt);
                        break;
            }
            break;
        case PRINTF_STATE_LENGTH:
            switch(*fmt) {
            case 'h':   length = PRINTF_LENGTH_SHORT;
                        state = PRINTF_STATE_LENGTH_SHORT;
                        break;
            case 'l':   length = PRINTF_LENGTH_LONG;
                        state = PRINTF_STATE_LENGTH_LONG;                   
                        break;
            default:    goto PRINTF_STATE_SPEC_;
            }
            break;
        case PRINTF_STATE_LENGTH_SHORT:
            if (*fmt == 'h') {
                length = PRINTF_LENGTH_SHORT_SHORT;
                state = PRINTF_STATE_SPEC;
            }
            else {
                goto PRINTF_STATE_SPEC_;
            }
            break;
        case PRINTF_STATE_LENGTH_LONG:
            if (*fmt == 'l') {
                length = PRINTF_LENGTH_LONG_LONG;
                state = PRINTF_STATE_SPEC;
            }
            else {
                goto PRINTF_STATE_SPEC_;
            }
            break;
        case PRINTF_STATE_SPEC:
        PRINTF_STATE_SPEC_:
            switch (*fmt) {
            case 'c':   putc((char)*argp);
                        argp++;
                        break;
            case 's':   puts(*(char**)argp);
                        argp++;
                        break;
            case '%':   putc('%');
                        break;
            case 'd':
            case 'i':   radix = 10; sign = true;
                        argp = printf_number(argp, length, sign, radix);
                        break;
            case 'u':   radix = 10; sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
            case 'X':
            case 'x':
            case 'p':   radix = 16; sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
            case 'o':   radix = 8; sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
            default:    break; /* ignore any invalid specifiers */
            }
            /* reset state */
            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            radix = 10;
            sign = false;
            break;
        }/* switch(state) */
        fmt++;
    }
}

/*----------------------------------------------------------------------------*/
const char g_hex_chars[] = "0123456789abcdef";
int* printf_number(int *argp, int length, bool sign, int radix)
{
    char buffer[32];
    unsigned long long number;
    int number_sign = 1;
    int pos = 0;

    switch(length) {
        case PRINTF_LENGTH_DEFAULT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_SHORT_SHORT:
            if (sign) {
                int n = *argp;
                if (n<0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else {
                number = *(unsigned int*)argp;
            }
            argp++;
            break;
        case PRINTF_LENGTH_LONG:
            if (sign) {
                long int n = *(long int*)argp;
                if (n<0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else {
                number = *(unsigned long int*)argp;
            }
            argp += 2;
            break;
        case PRINTF_LENGTH_LONG_LONG:
            if (sign) {
                long long int n = *(long long int*)argp;
                if (n<0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else {
                number = *(unsigned long long int*)argp;
            }
            argp += 4;
            break;
    }/* switch(length) */

    /* convert to ASCII */
    do {
        uint32_t remainder;
        x86_div64_32(number, radix, &number, &remainder);  /* remainder = number % radix; number = numer / radix */
        buffer[pos++] = g_hex_chars[remainder];
    } while (number > 0);
    /* add sign */
    if (sign && number_sign < 0) {
        buffer[pos++] = '-';
    }
    /* print in reverse order */
    while (--pos >= 0)
        putc(buffer[pos]);

    return argp;
}
