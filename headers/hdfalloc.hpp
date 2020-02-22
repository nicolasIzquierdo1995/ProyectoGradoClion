// Header file:
#include <stdio.h>
#include <string.h>

using namespace std;
namespace hdfalloc{
    void * HDmemfill(void * dest, const void * src, int item_size, int num_items);
    char * HIstrncpy(char *dest, const char *source, int len);
    void * HDmalloc(int qty);
    void * HDrealloc(void * where, int qty);
    void HDfree(void * ptr);
    void * HDcalloc(int n, int size);
    char * HDstrdup(const char *s);
}

#ifndef HDmemcpy
#define HDmemcpy(X,Y,Z)		memcpy((char*)(X),(const char*)(Y),Z)
#endif /* HDmemcpy */