// Header file:
#include <stdio.h>
#include <string.h>

using namespace std;
namespace hdfalloc{
    void * HDmalloc(int qty);
    void * HDrealloc(void * where, int qty);
    char * HDstrdup(const char *s);
}

#ifndef HDmemcpy
#define HDmemcpy(X,Y,Z)		memcpy((char*)(X),(const char*)(Y),Z)
#endif /* HDmemcpy */