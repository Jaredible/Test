#ifdef extern
#undef extern
#define EXTERN_FLAG
#endif

#include <stdio.h>

#ifdef EXTERN_FLAG
#define extern
#endif

extern int x
#ifdef extern
    = 2
#endif
    ;

extern float y;

extern double z
#ifdef extern
    = 42.0
#endif
    ;

