#include <stdarg.h>
#include <stdbool.h>
#include "stdio.h"
//#include "x86.h"

/*----------------------------------------------------------------------------*/
#define TAB_STOPS 4

/*----------------------------------------------------------------------------*/
uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;

const unsigned SCREEN_WIDTH = 80;
uint8_t g_ScreenPos_X = 0;

const unsigned SCREEN_HEIGHT = 25;
uint8_t g_ScreenPos_Y = 0;

const uint8_t DEFAULT_COLOR = 0x07;

/*------------------------------------------------------------------------------
** FORWARD DECLARATIONS
*/
void putchr(int x, int y, const char c);
void putcolor(int x, int y, uint8_t color);

/*----------------------------------------------------------------------------*/
void clrscr()
{
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, ' ');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
}
/*----------------------------------------------------------------------------*/
void putchr(int x, int y, const char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

/*----------------------------------------------------------------------------*/
void putcolor(int x, int y, uint8_t color)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

/*----------------------------------------------------------------------------*/
void putc(const char c)
{
    switch (c) {
        case '\n':
            g_ScreenPos_Y++;
            // fall through to case '\r'...
        case '\r':
            g_ScreenPos_X = 0;
            break;
        case '\t':
            for (int i = 0; i < 4 - (g_ScreenPos_X % TAB_STOPS); i++) {
                putc(' ');
            }
            break;
        default:
            putchr(g_ScreenPos_X, g_ScreenPos_Y, c);
            g_ScreenPos_X++;
            break;    
    }
    if (g_ScreenPos_X >= SCREEN_WIDTH) {
        g_ScreenPos_Y++;
        g_ScreenPos_X = 0;
    }
}/* putc() */

/*----------------------------------------------------------------------------*/
void puts(const char* str)
{
    while (*str) {
        putc(*str);
        str++;
    }
}


/*----------------------------------------------------------------------------*/
const char g_hex_chars[] = "0123456789abcdef";

void printf_unsigned(unsigned long long number, int radix)
{
    char buffer[32];
    int  pos = 0;

    /* convert number to ASCII */
    do {
        unsigned long long remainder = number % radix;
        number /= radix;
        buffer[pos++] = g_hex_chars[remainder];
    } while (number > 0);

    /* print number in reverse order */
    while (--pos >= 0)
        putc(buffer[pos]);
}

/*----------------------------------------------------------------------------*/
void printf_signed(long long number, int radix)
{
    if (number < 0 ) {
            putc('-');
            printf_unsigned(-number, radix);
    }
    else printf_unsigned(number, radix);
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

void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int  state = PRINTF_STATE_NORMAL;
    int  length = PRINTF_LENGTH_DEFAULT;
    int  radix = 10;
    bool sign = false;
    bool numerical = false;

    while (*fmt) {
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
                    case 'c':   putc((char)va_arg(args, int));
                                break;
                    case 's':   puts(va_arg(args, const char*));
                                break;
                    case '%':   putc('%');
                                break;
                    case 'd':
                    case 'i':   radix = 10; 
                                sign = true;
                                numerical = true;
                                break;
                    case 'u':   radix = 10; 
                                sign = false;
                                numerical = true;
                                break;
                    case 'X':
                    case 'x':
                    case 'p':   radix = 16; 
                                sign = false;
                                numerical = true;
                                break;
                    case 'o':   radix = 8; 
                                sign = false;
                                numerical = true;
                                break;
                    default:    break; /* ignore any invalid specifiers */
                }

                if (numerical) {
                    if (sign) {
                        switch (length) {
                        case PRINTF_LENGTH_DEFAULT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_SHORT_SHORT:
                            printf_signed(va_arg(args, int), radix);
                            break;
                        case PRINTF_LENGTH_LONG:
                            printf_signed(va_arg(args, long), radix);
                            break;
                        case PRINTF_LENGTH_LONG_LONG:
                            printf_signed(va_arg(args, long long), radix);
                            break;
                        default:
                            break;
                        }
                    }
                    else {
                        switch (length) {
                        case PRINTF_LENGTH_DEFAULT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_SHORT_SHORT:
                            printf_unsigned(va_arg(args, unsigned int), radix);
                            break;
                        case PRINTF_LENGTH_LONG:
                            printf_unsigned(va_arg(args, unsigned long), radix);
                            break;
                        case PRINTF_LENGTH_LONG_LONG:
                            printf_unsigned(va_arg(args, unsigned long long), radix);
                            break;
                        default:
                            break;
                        }
                    }
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
    va_end(args);
}/* printf() */
