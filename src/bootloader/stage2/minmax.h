#pragma once

/* 
**  shouldn't have (a) and (b) twice on same line, since (for 
**  example) min(a++,b) will increment (a) twice!
*/
#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))