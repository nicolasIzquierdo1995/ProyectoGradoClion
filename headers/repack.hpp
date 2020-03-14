#define H5FOPENERROR "unable to open file"
#define FAIL 0
#define PFORMAT  "%-7s %-7s %-7s\n"   /* chunk info, compression info, name*/
#define PFORMAT1 "%-7s %-7s %-7s"     /* chunk info, compression info, name*/
#define MAX_NC_NAME 256               /* max length of a name */
#define MAX_VAR_DIMS 32               /* max per variable dimensions */
#define FORMAT_OBJ      " %-27s %s\n"   /* obj type, name */
#define FORMAT_OBJ_ATTR "  %-27s %s\n"  /* obj type, name */
#define CD_VALUES 20
#define NELMTS(X)    	(sizeof(X)/sizeof(X[0]))
#define H5TOOLS_MALLOCSIZE      (128 * 1024 * 1024)
#define O_RDONLY	     00
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define HDopen(S,F,M)       open(S,F,M)
#undef MAX
#define MAX(a,b)		(((a)>(b)) ? (a) : (b))

#include "hdf5.h"
#include <string>

using namespace std;

typedef struct {
    char obj[MAX_NC_NAME];
} obj_list_t;


typedef enum {
    H5TRAV_TYPE_UNKNOWN = -1,        /* Unknown object type */
    H5TRAV_TYPE_GROUP,          /* Object is a group */
    H5TRAV_TYPE_DATASET,        /* Object is a dataset */
    H5TRAV_TYPE_NAMED_DATATYPE, /* Object is a named datatype */
    H5TRAV_TYPE_LINK,           /* Object is a symbolic link */
    H5TRAV_TYPE_UDLINK          /* Object is a user-defined link */
} h5trav_type_t;

typedef struct {
    H5Z_filter_t filtn;                           /* filter identification number */
    unsigned     cd_values[CD_VALUES];            /* filter client data values */
    size_t       cd_nelmts;                       /* filter client number of values */
} filter_info_t;

/* chunk lengths along each dimension and rank */
typedef struct {
    hsize_t chunk_lengths[MAX_VAR_DIMS];
    int     rank;
} chunk_info_t;

/* we currently define a maximum value for the filters array,
   that corresponds to the current library filters */
#define H5_REPACK_MAX_NFILTERS 6

/* information for one object, contains PATH, CHUNK info and FILTER info */
typedef struct {
    char          path[MAX_NC_NAME];               /* name of object */
    filter_info_t filter[H5_REPACK_MAX_NFILTERS];  /* filter array */
    int           nfilters;                        /* current number of filters */
    H5D_layout_t  layout;                          /* layout information */
    chunk_info_t  chunk;                           /* chunk information */
    hid_t         refobj_id;                       /* object ID, references */
} pack_info_t;

/* Typedefs for visiting objects */
typedef herr_t (*h5trav_obj_func_t)(const char *path_name, const H5O_info_t *oinfo,
                                    const char *first_seen, void *udata);
typedef herr_t (*h5trav_lnk_func_t)(const char *path_name, const H5L_info_t *linfo,
                                    void *udata);

static const char *drivernames[]={
        "sec2",
        "family",
        "split",
        "multi",
#ifdef H5_HAVE_STREAM
        "stream",
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
"mpio",
    "mpiposix"
#endif /* H5_HAVE_PARALLEL */
};

enum {
    SEC2_IDX = 0
    ,FAMILY_IDX
    ,SPLIT_IDX
    ,MULTI_IDX
#ifdef H5_HAVE_STREAM
    ,STREAM_IDX
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
    ,MPIO_IDX
   ,MPIPOSIX_IDX
#endif /* H5_HAVE_PARALLEL */
} driver_idx;
#define NUM_DRIVERS     (sizeof(drivernames) / sizeof(drivernames[0]))

/* store a table of all objects */
typedef struct {
    unsigned int size;
    unsigned int nelems;
    pack_info_t  *objs;
} pack_opttbl_t;

typedef struct bupBup {
    haddr_t addr;
    char *path;
} bupBup;

enum {
    no_arg = 0,         /* doesn't take an argument     */
    require_arg,        /* requires an argument	        */
    optional_arg,
};

typedef struct trav_addr_t {
    size_t      nalloc;
    size_t      nused;
    bupBup *objs;
} trav_addr_t;

typedef struct {
    h5trav_obj_func_t visit_obj;        /* Callback for visiting objects */
    h5trav_lnk_func_t visit_lnk;        /* Callback for visiting links */
    void *udata;                /* User data to pass to callbacks */
} trav_visitor_t;

typedef struct {
    trav_addr_t *seen;              /* List of addresses seen already */
    const trav_visitor_t *visitor;  /* Information for visiting each link/object */
    hbool_t is_absolute;            /* Whether the traversal has absolute paths */
    const char *base_grp_name;      /* Name of the group that serves as the base
                                     * for iteration */
} trav_ud_traverse_t;

typedef struct {
    hid_t fid;                      /* File ID being traversed */
} trav_print_udata_t;

/*-------------------------------------------------------------------------
 * public struct to store name and type of an object
 *-------------------------------------------------------------------------
 */
/* Struct to keep track of symbolic link targets visited.
 * Functions: symlink_visit_add() and symlink_is_visited()
 */

typedef struct cuco {
    H5L_type_t  type;
    char *file;
    char *path;
} cuco;
typedef struct symlink_trav_t {
    size_t      nalloc;
    size_t      nused;
    cuco *objs;
    hbool_t dangle_link;
} symlink_trav_t;

typedef struct trav_path_t {
    char      *path;
    h5trav_type_t type;
} trav_path_t;

typedef struct long_options {
    char  *name;
    int          has_arg;       /* whether we should look for an arg    */
    char         shortval;      /* the shortname equivalent of long arg
                                 * this gets returned from get_option   */
} long_options;

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
 * command line options
 *-------------------------------------------------------------------------
 */

/* all the above, ready to go to the hrepack call */
typedef struct {
    pack_opttbl_t   *op_tbl;     /*table with all -c and -f options */
    int             all_layout;  /*apply the layout to all objects */
    int             all_filter;  /*apply the filter to all objects */
    filter_info_t   filter_g[H5_REPACK_MAX_NFILTERS];    /*global filter array for the ALL case */
    int             n_filter_g;  /*number of global filters */
    chunk_info_t    chunk_g;     /*global chunk INFO for the ALL case */
    H5D_layout_t    layout_g;    /*global layout information for the ALL case */
    int             verbose;     /*verbose mode */
    hsize_t         min_comp;    /*minimum size to compress, in bytes */
    int             use_native;  /*use a native type in write */
    int             latest;      /*pack file with the latest file format */
    int             grp_compact; /* Set the maximum number of links to store as header messages in the group */
    int             grp_indexed; /* Set the minimum number of links to store in the indexed format */
    int             msg_size[8]; /* Minimum size of shared messages: dataspace,
                                 datatype, fill value, filter pipleline, attribute */
    const char      *ublock_filename; /* user block file name */
    hsize_t         ublock_size;      /* user block size */
    hsize_t         threshold;        /* alignment threshold for H5Pset_alignment */
    hsize_t         alignment ;       /* alignment for H5Pset_alignment */
} pack_opt_t;


typedef struct named_dt_t {
    haddr_t             addr_in;    /* Address of the named dtype in the in file */
    hid_t               id_out;     /* Open identifier for the dtype in the out file */
    struct named_dt_t   *next;      /* Next dtype */
} named_dt_t;


namespace h5repack {
    int noMain(string inputFile, string outputFile, string gzipCompression);
}