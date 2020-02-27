
#ifndef PROYECTOGRADOCLION_H5_REPACK_COPY_H
#define PROYECTOGRADOCLION_H5_REPACK_COPY_H

#endif //PROYECTOGRADOCLION_H5_REPACK_COPY_H

#include "hdf5.h"
#include "h5trav.hpp"
#include "utils.hpp"
using namespace H5;


#define H5_REPACK_MAX_NFILTERS 6 /* we currently define a maximum value for the filters array,
   that corresponds to the current library filters */
#define CD_VALUES 20
#define MAX_VAR_DIMS 32               /* max per variable dimensions */
#define MAX_NC_NAME 256               /* max length of a name */
#define NELMTS(X)    (sizeof(X)/sizeof(X[0]))
const hsize_t H5TOOLS_BUFSIZE = 33554432;  /* 32 MB */

namespace h5repack {

    typedef struct {
        H5Z_filter_t filtn;                           /* filter identification number */
        unsigned cd_values[CD_VALUES];            /* filter client data values */
        size_t cd_nelmts;                       /* filter client number of values */
    } filter_info_t;

/* chunk lengths along each dimension and rank */
    typedef struct {
        hsize_t chunk_lengths[MAX_VAR_DIMS];
        int rank;
    } chunk_info_t;

/* information for one object, contains PATH, CHUNK info and FILTER info */
    typedef struct {
        char path[MAX_NC_NAME];               /* name of object */
        filter_info_t filter[H5_REPACK_MAX_NFILTERS];  /* filter array */
        int nfilters;                        /* current number of filters */
        H5D_layout_t layout;                          /* layout information */
        chunk_info_t chunk;                           /* chunk information */
        hid_t refobj_id;                       /* object ID, references */
    } pack_info_t;

/* store a table of all objects */
    typedef struct {
        unsigned int size;
        unsigned int nelems;
        pack_info_t *objs;
    } pack_opttbl_t;

    typedef struct {
        pack_opttbl_t *op_tbl;     /*table with all -c and -f options */
        int all_layout;  /*apply the layout to all objects */
        int all_filter;  /*apply the filter to all objects */
        filter_info_t filter_g[H5_REPACK_MAX_NFILTERS];    /*global filter array for the ALL case */
        int n_filter_g;  /*number of global filters */
        chunk_info_t chunk_g;     /*global chunk INFO for the ALL case */
        H5D_layout_t layout_g;    /*global layout information for the ALL case */
        int verbose;     /*verbose mode */
        hsize_t min_comp;    /*minimum size to compress, in bytes */
        int use_native;  /*use a native type in write */
        int latest;      /*pack file with the latest file format */
        int grp_compact; /* Set the maximum number of links to store as header messages in the group */
        int grp_indexed; /* Set the minimum number of links to store in the indexed format */
        int msg_size[8]; /* Minimum size of shared messages: dataspace,
                                 datatype, fill value, filter pipleline, attribute */
        const char *ublock_filename; /* user block file name */
        hsize_t ublock_size;      /* user block size */
        hsize_t threshold;        /* alignment threshold for H5Pset_alignment */
        hsize_t alignment;       /* alignment for H5Pset_alignment */
    } pack_opt_t;

    typedef struct named_dt_t {
        haddr_t addr_in;
        hid_t id_out;
        struct named_dt_t *next;
    } named_dt_t;

    int copy_objects(H5File fidin,const char* fnameout);
}
