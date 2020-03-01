#ifdef MALDEBUG
#define __MALDEBUG__
#endif

#include "../headers/hdfalloc.hpp"
#include <cstdlib>
#include <cstring>
using namespace hdfalloc;

void * hdfalloc::HDmalloc(int qty)
{
    char *p = (char *) malloc(qty);
    return(p);
}


void * hdfalloc::HDrealloc(void * where, int qty)
{
    char *p = (char *) realloc(where, qty);
    return(p);
}

char* hdfalloc::HDstrdup(const char *s)
{
    char *ret;

    if (s == NULL)
        return(NULL);


    ret = (char *) HDmalloc((int) strlen(s) + 1);
    if (ret == NULL)
        return (NULL);


    strcpy(ret, s);
    return (ret);
}