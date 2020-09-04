#define FAIL 0
#define MAX_NC_NAME 256               /* max length of a name */
#define MAX_VAR_DIMS 32               /* max per variable dimensions */
#define CD_VALUES 20
#define NELMTS(X)    	(sizeof(X)/sizeof(X[0]))
#define H5TOOLS_MALLOCSIZE      (128 * 1024 * 1024)
#undef MAX
#define MAX(a,b)		(((a)>(b)) ? (a) : (b))

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include "H5Cpp.h"

using namespace std;
using namespace H5;

typedef struct {
    char obj[MAX_NC_NAME];
} obj_list_t;

typedef enum {
    H5TRAV_TYPE_UNKNOWN = -1,
    H5TRAV_TYPE_GROUP,
    H5TRAV_TYPE_DATASET,
    H5TRAV_TYPE_NAMED_DATATYPE,
    H5TRAV_TYPE_LINK,
    H5TRAV_TYPE_UDLINK
} h5trav_type_t;

typedef struct {
    H5Z_filter_t filtn;
    unsigned     cd_values[CD_VALUES];
    size_t       cd_nelmts;
} filter_info_t;

typedef struct {
    hsize_t chunk_lengths[MAX_VAR_DIMS];
    int     rank;
} chunk_info_t;

#define H5_REPACK_MAX_NFILTERS 6

typedef struct {
    char          path[MAX_NC_NAME];
    filter_info_t filter[H5_REPACK_MAX_NFILTERS];
    int           nfilters;
    H5D_layout_t  layout;
    chunk_info_t  chunk;
    hid_t         refobj_id;
} pack_info_t;

typedef herr_t (*h5trav_obj_func_t)(const char *path_name, const H5O_info_t *oinfo, const char *first_seen, void *udata);
typedef herr_t (*h5trav_lnk_func_t)(const char *path_name, const H5L_info_t *linfo, void *udata);

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
    no_arg = 0,
    optional_arg,
};

typedef struct trav_addr_t {
    size_t      nalloc;
    size_t      nused;
    bupBup *objs;
} trav_addr_t;

typedef struct {
    h5trav_obj_func_t visit_obj;
    h5trav_lnk_func_t visit_lnk;
    void *udata;
} trav_visitor_t;

typedef struct {
    trav_addr_t *seen;
    const trav_visitor_t *visitor;
    hbool_t is_absolute;
    const char *base_grp_name;
} trav_ud_traverse_t;

typedef struct long_options {
    char  *name;
    int          has_arg;
    char         shortval;
} long_options;

typedef struct trav_link_t {
    char      *new_name;
} trav_link_t;

typedef struct trav_obj_t {
    haddr_t     objno;
    unsigned    flags[2];
    char        *name;
    h5trav_type_t type;
    trav_link_t *links;
    size_t      sizelinks;
    size_t      nlinks;
} trav_obj_t;

typedef struct trav_table_t {
    size_t      size;
    size_t      nobjs;
    trav_obj_t *objs;
} trav_table_t;

typedef struct {
    pack_opttbl_t   *op_tbl;
    int             all_layout;
    int             all_filter;
    filter_info_t   filter_g[H5_REPACK_MAX_NFILTERS];
    int             n_filter_g;
    chunk_info_t    chunk_g;
    H5D_layout_t    layout_g;
    int             verbose;
    hsize_t         min_comp;
    int             use_native;
    int             latest;
    int             grp_compact;
    int             grp_indexed;
    int             msg_size[8];
    const char      *ublock_filename;
    hsize_t         ublock_size;
    hsize_t         threshold;
    hsize_t         alignment ;
} pack_opt_t;

typedef struct named_dt_t {
    haddr_t             addr_in;
    hid_t               id_out;
    struct named_dt_t   *next;
} named_dt_t;

namespace h5repack {
    int repack(H5File* inputFile, string outputFile, string gzipCompression);
}