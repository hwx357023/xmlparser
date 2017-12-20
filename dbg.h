#ifndef __dbg_h
#define __dbg_h

#include <stdio.h>

#define NDEBUG

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stdout, "DEBUG %s (in function '%s'):%d:  " M "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#endif
