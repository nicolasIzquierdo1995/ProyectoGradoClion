#ifndef H5TRAV_H__
#define H5TRAV_H__

#include "hdf5.h"

#ifdef H5_STDC_HEADERS
#   include <assert.h>
#   include <ctype.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <float.h>
#   include <limits.h>
#   include <math.h>
#   include <signal.h>
#   include <stdarg.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#endif



/* Typedefs for visiting objects */
typedef herr_t (*h5trav_obj_func_t)(const char *path_name, const H5O_info_t *oinfo,
        const char *first_seen, void *udata);
typedef herr_t (*h5trav_lnk_func_t)(const char *path_name, const H5L_info_t *linfo,
        void *udata);

typedef enum {
    H5TRAV_TYPE_UNKNOWN = -1,        /* Unknown object type */
    H5TRAV_TYPE_GROUP,          /* Object is a group */
    H5TRAV_TYPE_DATASET,        /* Object is a dataset */
    H5TRAV_TYPE_NAMED_DATATYPE, /* Object is a named datatype */
    H5TRAV_TYPE_LINK,           /* Object is a symbolic link */
    H5TRAV_TYPE_UDLINK          /* Object is a user-defined link */
} h5trav_type_t;

/* maximum of two, three, or four values */
#undef MAX
#define MAX(a,b)		(((a)>(b)) ? (a) : (b))


/*-------------------------------------------------------------------------
 * keep record of hard link information
 *-------------------------------------------------------------------------
 */
typedef struct trav_link_t {
    char      *new_name;
} trav_link_t;


/*-------------------------------------------------------------------------
 * struct to store basic info needed for the h5trav table traversal algorythm
 *-------------------------------------------------------------------------
 */

typedef struct trav_obj_t {
    haddr_t     objno;     /* object address */
    unsigned    flags[2];  /* h5diff.object is present or not in both files*/
    char        *name;     /* name */
    h5trav_type_t type;    /* type of object */
    trav_link_t *links;    /* array of possible link names */
    size_t      sizelinks; /* size of links array */
    size_t      nlinks;    /* number of links */
} trav_obj_t;


/*-------------------------------------------------------------------------
 * private struct that stores all objects
 *-------------------------------------------------------------------------
 */

typedef struct trav_table_t {
    size_t      size;
    size_t      nobjs;
    trav_obj_t *objs;
} trav_table_t;


/*-------------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

namespace h5trav {

/*-------------------------------------------------------------------------
 * "h5trav table" public functions
 *-------------------------------------------------------------------------
 */

int h5trav_gettable(hid_t fid, trav_table_t *travt);

/*-------------------------------------------------------------------------
 * "h5trav print" public functions
 *-------------------------------------------------------------------------
 */

#ifdef __cplusplus
}
#endif


/*-------------------------------------------------------------------------
 * table private functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL void trav_table_init(trav_table_t **table);

}


#ifndef HDstrcat
#define HDstrcat(X,Y)		strcat(X,Y)
#endif /* HDstrcat */
#ifndef HDstrchr
#define HDstrchr(S,C)		strchr(S,C)
#endif /* HDstrchr */
#ifndef HDstrcmp
#define HDstrcmp(X,Y)		strcmp(X,Y)
#endif /* HDstrcmp */
#ifndef HDstrcasecmp
#define HDstrcasecmp(X,Y)       strcasecmp(X,Y)
#endif /* HDstrcasecmp */
#ifndef HDstrcoll
#define HDstrcoll(X,Y)		strcoll(X,Y)
#endif /* HDstrcoll */
#ifndef HDstrcpy
#define HDstrcpy(X,Y)		strcpy(X,Y)
#endif /* HDstrcpy */
#ifndef HDstrcspn
#define HDstrcspn(X,Y)		strcspn(X,Y)
#endif /* HDstrcspn */
#ifndef HDstrerror
#define HDstrerror(N)		strerror(N)
#endif /* HDstrerror */
#ifndef HDstrftime
#define HDstrftime(S,Z,F,T)	strftime(S,Z,F,T)
#endif /* HDstrftime */
#ifndef HDstrlen
#define HDstrlen(S)		strlen(S)
#endif /* HDstrlen */
#ifndef HDstrncat
#define HDstrncat(X,Y,Z)	strncat(X,Y,Z)
#endif /* HDstrncat */
#ifndef HDstrncmp
#define HDstrncmp(X,Y,Z)	strncmp(X,Y,Z)
#endif /* HDstrncmp */
#ifndef HDstrncpy
#define HDstrncpy(X,Y,Z)	strncpy(X,Y,Z)
#endif /* HDstrncpy */
#ifndef HDstrpbrk
#define HDstrpbrk(X,Y)		strpbrk(X,Y)
#endif /* HDstrpbrk */
#ifndef HDstrrchr
#define HDstrrchr(S,C)		strrchr(S,C)
#endif /* HDstrrchr */
#ifndef HDstrspn
#define HDstrspn(X,Y)		strspn(X,Y)
#endif /* HDstrspn */
#ifndef HDstrstr
#define HDstrstr(X,Y)		strstr(X,Y)
#endif /* HDstrstr */
#ifndef HDstrtod
#define HDstrtod(S,R)		strtod(S,R)
#endif /* HDstrtod */
#ifndef HDstrtok
#define HDstrtok(X,Y)		strtok(X,Y)
#endif /* HDstrtok */
#ifndef HDstrtol
#define HDstrtol(S,R,N)		strtol(S,R,N)
#endif /* HDstrtol */

#endif  /* H5TRAV_H__ */