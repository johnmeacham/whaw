#ifndef _H_UTIL
#define _H_UTIL
/* arch-tag: c715407e-864b-4faa-b016-89eb6fd4de5c */

#include <stdbool.h>

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
#define BOUND(min,max,val) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#define MIN3(a,b,c) ((a)<(b)?MIN(a,c):MIN(b,c))
#define MAX3(a,b,c) ((a)>(b)?MAX(a,c):MAX(b,c))

#endif

