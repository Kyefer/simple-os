#define LOCAL

#ifdef LOCAL

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum bool_e { false,
                      true } bool;

#define MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define MALLOC(n) malloc((n))
#define MEMSET(dst, val, n) memset((dst), (val), (n))
#define FREE(ptr) free((ptr))

#define STRCPY(dst, src) strcpy((dst), (src))
#define STRCMP(a, b) strcmp((a), (b))
#define STRLEN(str) strlen((str))

#define PRINTF(frmt, ...) printf((frmt), __VA_ARGS__)
#define PRINT(str) printf((str))

#define PSTR(str) printf("%s\n", str)
#define PNUM(num) printf("%d\n", num)

#else

#define MEMCPY(dst, src, n) _kmemcpy((dst), (src), (n))
#define MALLOC(n) _kmalloc((n))
#define MEMSET(dst, val, n) _kmemset((dst), (n), (val))
#define FREE(ptr) _kfree((ptr))

#define STRCPY(dst, src) _kstrcpy((dst), (src))
#define STRCMP(a, b) _kstrcmp((a), (b))
#define STRLEN(str) _kstrlen((str))

#define PRINTF(frmt, ...) __cio_printf((frmt), __VA_ARGS__)
#define PRINT(str) __cio_puts(str)

#endif
