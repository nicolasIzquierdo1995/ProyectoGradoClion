#ifdef MALDEBUG
#define __MALDEBUG__
#endif

#include "../headers/hdfalloc.hpp"
#include <cstdlib>
#include <cstring>
using namespace hdfalloc;
/*
EXPORTED ROUTINES
  HDmemfill    -- copy a chunk of memory repetitively into another chunk
  HIstrncpy    -- string copy with termination
  HDmalloc     -- dynamicly allocates memory
  HDrealloc    -- dynamicly resize (reallocate) memory
  HDfree       -- free dynamicly allocated memory
  HDcalloc     -- dynamicly allocates memory and clears it to zero
  HDstrdup     -- in-library replacement for non-ANSI strdup()
*/


/*--------------------------------------------------------------------------
 NAME
    HDmalloc -- dynamicly allocates memory
 USAGE
    void * HDmalloc(qty)
        uint32 qty;         IN: the (minimum) number of bytes to allocate in
                                the memory block.
 RETURNS
    Pointer to the memory allocated on success, NULL on failure.
 DESCRIPTION
    Dynamicly allocates a block of memory and returns a pointer to it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Acts like malloc().
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void * hdfalloc::HDmalloc(int qty)
{
    char FUNC[]="HDmalloc";
    char *p;

    p = (char *) malloc(qty);
    if (p== (char *) NULL) {
      } /* end if */
    return(p);
}   /* end HDmalloc() */

/*--------------------------------------------------------------------------
 NAME
    HDrealloc -- dynamicly resize (reallocate) memory
 USAGE
    void * HDrealloc(vfp,qty)
        void * vfp;          IN: pointer to the memory block to resize.
        uint32 qty;         IN: the (minimum) number of bytes to allocate in
                                the new memory block.
 RETURNS
    Pointer to the memory allocated on success, NULL on failure.
 DESCRIPTION
    Dynamicly re-allocates a block of memory and returns a pointer to it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Acts like realloc().
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void * hdfalloc::HDrealloc(void * where, int qty)
{
    char FUNC[]="HDrealloc";
    char *p;

    p = (char *) realloc(where, qty);
    if (p== (char *) NULL) {
      } /* end if */
    return(p);
}   /* end HDrealloc() */

/*--------------------------------------------------------------------------
 NAME
    HDfree -- free dynamicly allocated memory
 USAGE
    void HDfree(vfp)
        void * vfp;          IN: pointer to the memory block to free.
 RETURNS
    NULL?
 DESCRIPTION
    Free dynamicly allocated blocks of memory.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Acts like free().
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void hdfalloc::HDfree(void * ptr)
{
    if (ptr!=NULL)
        free(ptr);
}   /* end HDfree() */


/*--------------------------------------------------------------------------
 NAME
    HDstrdup -- in-library replacement for non-ANSI strdup()
 USAGE
    char *HDstrdup(s)
        const char *s;          IN: pointer to the string to duplicate
 RETURNS
    Pointer to the duplicated string, or NULL on failure.
 DESCRIPTION
    Duplicates a string (i.e. allocates space and copies it over).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Acts like strdup().
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char* hdfalloc::HDstrdup(const char *s)
{
    char *ret;

    /* Make sure original string is not NULL */
    if (s == NULL)
        return(NULL);

    /* Allocate space */
    ret = (char *) HDmalloc((int) strlen(s) + 1);
    if (ret == NULL)
        return (NULL);

    /* Copy the original string and return it */
    strcpy(ret, s);
    return (ret);
}   /* end HDstrdup() */