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
 * public struct to store name and type of an object
 *-------------------------------------------------------------------------
 */
/* Struct to keep track of symbolic link targets visited.
 * Functions: symlink_visit_add() and symlink_is_visited()
 */
typedef struct symlink_trav_t_obj {
    H5L_type_t  type;
    char *file;
    char *path;
} symlink_trav_t_obj;
typedef struct symlink_trav_t {
    size_t      nalloc;
    size_t      nused;
    symlink_trav_t_obj *objs;
    hbool_t dangle_link;
} symlink_trav_t;

typedef struct trav_path_t {
    char      *path;
    h5trav_type_t type;
} trav_path_t;

typedef struct trav_info_t {
    size_t      nalloc;
    size_t      nused;
    const char *fname;
    hid_t fid;                          /* File ID */
    trav_path_t *paths;
    symlink_trav_t symlink_visited;     /* already visited symbolic links */
    void * opts;                        /* optional data passing */
} trav_info_t;


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
 * "h5trav general" public functions
 *-------------------------------------------------------------------------
 */
    int h5trav_visit(hid_t file_id, const char *grp_name,
                                 hbool_t visit_start, hbool_t recurse, h5trav_obj_func_t visit_obj,
                                 h5trav_lnk_func_t visit_lnk, void *udata);

    herr_t symlink_visit_add(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path);

    hbool_t
    symlink_is_visited(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path);

/*-------------------------------------------------------------------------
 * "h5trav info" public functions
 *-------------------------------------------------------------------------
 */
    int h5trav_getinfo(hid_t file_id, trav_info_t *info);

    ssize_t h5trav_getindex(const trav_info_t *info, const char *obj);

    int
    trav_info_visit_obj(const char *path, const H5O_info_t *oinfo, const char *already_visited, void *udata);

    int trav_info_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata);

/*-------------------------------------------------------------------------
 * "h5trav table" public functions
 *-------------------------------------------------------------------------
 */

int h5trav_gettable(hid_t fid, trav_table_t *travt);

int h5trav_getindext(const char *obj, const trav_table_t *travt);

/*-------------------------------------------------------------------------
 * "h5trav print" public functions
 *-------------------------------------------------------------------------
 */
    H5TOOLS_DLL int h5trav_print(hid_t fid);

#ifdef __cplusplus
}
#endif

/*-------------------------------------------------------------------------
 * info private functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL void trav_info_init(const char *filename, hid_t fileid, trav_info_t **info);

H5TOOLS_DLL void trav_info_free(trav_info_t *info);

H5TOOLS_DLL void trav_info_add(trav_info_t *info, const char *path, h5trav_type_t obj_type);

/*-------------------------------------------------------------------------
 * table private functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL void trav_table_init(trav_table_t **table);

H5TOOLS_DLL void trav_table_free(trav_table_t *table);

H5TOOLS_DLL void trav_table_addflags(unsigned *flags,
                                     char *objname,
                                     h5trav_type_t type,
                                     trav_table_t *table);
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