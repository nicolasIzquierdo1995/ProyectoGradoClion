#include "../headers/h5trav.hpp"
#include "../headers/h5_repack_copy.hpp"
#include "../headers/hdfalloc.hpp"
#include <unistd.h>
using namespace std;
using namespace H5;
using namespace h5trav;
using namespace hdfalloc;
using namespace h5repack;
#define USERBLOCK_XFER_SIZE 512

hid_t copy_named_datatype(hid_t type_in, hid_t fidout, named_dt_t **named_dt_head_p, trav_table_t *travt, pack_opt_t *options) {

    named_dt_t* dt = *named_dt_head_p; /* Stack pointer */
    named_dt_t*  dt_ret = NULL;     /* Datatype to return */
    H5O_info_t  oinfo;              /* Object info of input dtype */
    hid_t       ret_value = -1;     /* The identifier of the named dtype in the out file */

    if(H5Oget_info(type_in, &oinfo) < 0)
        return(-1);

    if(*named_dt_head_p)
    {
        /* Stack already exists, search for the datatype */
        while(dt && dt->addr_in != oinfo.addr)
            dt = dt->next;

        dt_ret = dt;
    }
    else
    {
        /* Create the stack */
        size_t  i;

        for(i=0; i<travt->nobjs; i++)
            if(travt->objs[i].type == H5TRAV_TYPE_NAMED_DATATYPE)
            {
                /* Push onto the stack */
                dt->next = *named_dt_head_p;
                *named_dt_head_p = dt;

                /* Update the address and id */
                dt->addr_in = travt->objs[i].objno;
                dt->id_out = -1;

                /* Check if this type is the one requested */
                if(oinfo.addr == dt->addr_in)
                {
                    assert(!dt_ret);
                    dt_ret = dt;
                } /* end if */
            } /* end if */
    } /* end else */

    /* Handle the case that the requested datatype was not found.  This is
     * possible if the datatype was committed anonymously in the input file. */
    if(!dt_ret)
    {
        /* Push the new datatype onto the stack */
        NULL == (dt_ret = (named_dt_t *) malloc(sizeof(named_dt_t)));
        dt_ret->next = *named_dt_head_p;
        *named_dt_head_p = dt_ret;

        /* Update the address and id */
        dt_ret->addr_in = oinfo.addr;
        dt_ret->id_out = -1;
    } /* end if */

    /* If the requested datatype does not yet exist in the output file, copy it
     * anonymously */
    if(dt_ret->id_out < 0)
    {
        if (options->use_native==1)
            dt_ret->id_out = 1;//h5tools_get_native_type(type_in);
        else
            dt_ret->id_out = H5Tcopy(type_in);

        H5Tcommit_anon(fidout, dt_ret->id_out, H5P_DEFAULT, H5P_DEFAULT);
    } /* end if */

    /* Set return value */
    ret_value = dt_ret->id_out;

    /* Increment the ref count on id_out, because the calling function will try
     * to close it */
    if(H5Iinc_ref(ret_value) < 0)
        return(-1);

    return(ret_value);
}

void named_datatype_free(named_dt_t **named_dt_head_p, int ignore_err)
{
    named_dt_t *dt = *named_dt_head_p;

    while(dt)
    {
        /* Pop the datatype off the stack and free it */
        H5Tclose(dt->id_out);

        dt = dt->next;
        HDfree(*named_dt_head_p);
        *named_dt_head_p = dt;
    } /* end while */


} /* end named_datatype_free */

static int copy_attr_error(hid_t ftype_id, hid_t wtype_id, hid_t space_id, hid_t attr_id, hid_t attr_out, void* buf){
    H5Tclose(ftype_id);
    H5Tclose(wtype_id);
    H5Sclose(space_id);
    H5Aclose(attr_id);
    H5Aclose(attr_out);
    if (buf)
        free(buf);
    return -1;
}

int copy_attr(hid_t loc_in, hid_t loc_out, named_dt_t **named_dt_head_p, trav_table_t *travt, pack_opt_t *options) {
    hid_t      attr_id=-1;        /* attr ID */
    hid_t      attr_out=-1;       /* attr ID */
    hid_t      space_id=-1;       /* space ID */
    hid_t      ftype_id=-1;       /* file type ID */
    hid_t      wtype_id=-1;       /* read/write type ID */
    size_t     msize;             /* size of type */
    void*       buf=NULL;         /* data buffer */
    hsize_t    nelmts;            /* number of elements in dataset */
    int        rank;              /* rank of dataset */
    htri_t     is_named;          /* Whether the datatype is named */
    hsize_t    dims[H5S_MAX_RANK];/* dimensions of dataset */
    char       name[255];
    H5O_info_t oinfo;             /* object info */
    int        j;
    unsigned   u;

    if(H5Oget_info(loc_in, &oinfo) < 0)
        return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

    /*-------------------------------------------------------------------------
    * copy all attributes
    *-------------------------------------------------------------------------
    */

    for ( u = 0; u < (unsigned)oinfo.num_attrs; u++)
    {
        buf=NULL;

        /* open attribute */
        if((attr_id = H5Aopen_by_idx(loc_in, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t)u, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        /* get name */
        if (H5Aget_name( attr_id, (size_t)255, name ) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        /* get the file datatype  */
        if ((ftype_id = H5Aget_type( attr_id )) < 0 )
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        /* Check if the datatype is committed */
        if((is_named = H5Tcommitted(ftype_id)) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
        if(is_named && travt)
        {
            hid_t fidout;

            /* Create out file id */
            if((fidout = H5Iget_file_id(loc_out)) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

            /* Copy named dt */
            if((wtype_id = copy_named_datatype(ftype_id, fidout, named_dt_head_p,
                                               travt, options)) < 0)
            {
                H5Fclose(fidout);
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
            } /* end if */

            if(H5Fclose(fidout) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
        }
        else
        {
            if (options->use_native==1)
                wtype_id = 1;//h5tools_get_native_type(ftype_id);
            else
                wtype_id = H5Tcopy(ftype_id);
        } /* end if */

        /* get the dataspace handle  */
        if ((space_id = H5Aget_space( attr_id )) < 0 )
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        /* get dimensions  */
        if ( (rank = H5Sget_simple_extent_dims(space_id, dims, NULL)) < 0 )
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        nelmts=1;
        for (j=0; j<rank; j++)
            nelmts*=dims[j];

        if ((msize=H5Tget_size(wtype_id))==0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

        /*-------------------------------------------------------------------------
        * object references are a special case
        * we cannot just copy the buffers, but instead we recreate the reference
        * this is done on a second sweep of the file that just copies
        * the referenced objects
        *-------------------------------------------------------------------------
        */

        if (H5T_REFERENCE==H5Tget_class(wtype_id))
        {
            ;
        }
        else
        {
            /*-------------------------------------------------------------------------
            * read to memory
            *-------------------------------------------------------------------------
            */

            buf = (void *)malloc((size_t)(nelmts * msize));

            if(H5Aread(attr_id, wtype_id, buf) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

            /*-------------------------------------------------------------------------
            * copy
            *-------------------------------------------------------------------------
            */

            if((attr_out = H5Acreate2(loc_out, name, wtype_id, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
            if(H5Awrite(attr_out, wtype_id, buf) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);


            if(H5Aclose(attr_out) < 0)
                return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);


            if(buf)
                free(buf);

        }



        /*-------------------------------------------------------------------------
        * close
        *-------------------------------------------------------------------------
        */

        if (H5Tclose(ftype_id) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
        if (H5Tclose(wtype_id) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
        if (H5Sclose(space_id) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);
        if (H5Aclose(attr_id) < 0)
            return copy_attr_error(ftype_id, wtype_id, space_id, attr_id, attr_out, buf);

    } /* u */


    return 0;
}

static int copy_user_block(const char *infile, const char *outfile, hsize_t size)
{
    int infid = -1, outfid = -1;        /* File descriptors */
    int status = 0;                     /* Return value */

/* Open files */

    if((infid = open(infile, O_RDONLY, 0)) < 0) {
        status = -1;
        close(infid);
        close(outfid);
    }
    if((outfid = open(outfile, O_WRONLY, 0644)) < 0) {
        status = -1;

        close(infid);
        close(outfid);
    }

/* Copy the userblock from the input file to the output file */

    while(size > 0) {
        ssize_t nread, nbytes;
/* # of bytes transfered, etc. */

        char rbuf[USERBLOCK_XFER_SIZE];
/* Buffer for reading */

        const char *wbuf;
/* Pointer into buffer, for writing */



/* Read buffer from source file */

        if(size > USERBLOCK_XFER_SIZE)
            nread = read(infid, rbuf, (size_t)USERBLOCK_XFER_SIZE);
        else
            nread = read(infid, rbuf, (size_t)size);
        if(nread < 0) {
            status = -1;

            close(infid);
            close(outfid);
        }
/* end if */



/* Write buffer to destination file */


/* (compensating for interrupted writes & checking for errors, etc.) */

        nbytes = nread;
        wbuf = rbuf;
        while(nbytes > 0) {
            ssize_t nwritten;
/* # of bytes written */


            do {
                nwritten = write(outfid, wbuf, (size_t)nbytes);
            } while(-1 == nwritten && EINTR == errno);
            if(-1 == nwritten) {
/* error */

                status = -1;

                close(infid);
                close(outfid);
            }
/* end if */

            assert(nwritten > 0);
            assert(nwritten <= nbytes);


/* Update # of bytes left & offset in buffer */

            nbytes -= nwritten;
            wbuf += nwritten;
            assert(nbytes == 0 || wbuf < (rbuf + USERBLOCK_XFER_SIZE));
        }
/* end while */



/* Update size of userblock left to transfer */

        size -= nread;
    }
/* end while */


    close(infid);
    close(outfid);

    return status;
}

/*
int apply_filters(const char* name,    *//* object name from traverse list *//*
                  int rank,            *//* rank of dataset *//*
                  hsize_t dims,
size_t msize,        *//* size of type *//*
        hid_t dcpl_id,       *//* dataset creation property list *//*
pack_opt_t options,
int has_filter)


{
int          nfilters;       *//* number of filters in DCPL *//*
hsize_t      chsize[64];     *//* chunk size in elements *//*
H5D_layout_t layout;
int          i;
pack_info_t  obj;

*has_filter = 0;

if (rank==0) *//* scalar dataset, do not apply *//*
return 0;

*//*-------------------------------------------------------------------------
 * initialize the assigment object
 *-------------------------------------------------------------------------
 *//*
init_packobject(&obj);

*//*-------------------------------------------------------------------------
 * find options
 *-------------------------------------------------------------------------
 *//*
if (aux_assign_obj(name,options,&obj)==0)
return 0;

*//* get information about input filters *//*
if ((nfilters = H5Pget_nfilters(dcpl_id))<0)
return -1;

*//*-------------------------------------------------------------------------
 * check if we have filters in the pipeline
 * we want to replace them with the input filters
 * only remove if we are inserting new ones
 *-------------------------------------------------------------------------
 *//*
if (nfilters && obj.nfilters )
{
*has_filter = 1;
if (H5Premove_filter(dcpl_id,H5Z_FILTER_ALL)<0)
return -1;
}

*//*-------------------------------------------------------------------------
 * check if there is an existent chunk
 * read it only if there is not a requested layout
 *-------------------------------------------------------------------------
 *//*
if (obj.layout == -1 )
{
if ((layout = H5Pget_layout(dcpl_id))<0)
return -1;

if (layout == H5D_CHUNKED)
{
if ((rank = H5Pget_chunk(dcpl_id,NELMTS(chsize),chsize/out/))<0)
return -1;
obj.layout = H5D_CHUNKED;
obj.chunk.rank = rank;
for ( i = 0; i < rank; i++)
obj.chunk.chunk_lengths[i] = chsize[i];
}
}

if (obj.nfilters)
{

*//*-------------------------------------------------------------------------
 * filters require CHUNK layout; if we do not have one define a default
 *-------------------------------------------------------------------------
 *//*
if (obj.layout==-1)
{

*//* stripmine info *//*
hsize_t sm_size[H5S_MAX_RANK]; *//*stripmine size *//*
hsize_t sm_nbytes;             *//*bytes per stripmine *//*

obj.chunk.rank = rank;

*//*
* determine the strip mine size. The strip mine is
* a hyperslab whose size is manageable.
*//*



sm_nbytes = msize;
for ( i = rank; i > 0; --i)
{
hsize_t size = H5TOOLS_BUFSIZE / sm_nbytes;
if ( size == 0) *//* datum size > H5TOOLS_BUFSIZE *//*
size = 1;
sm_size[i - 1] = MIN(dims[i - 1], size);
sm_nbytes *= sm_size[i - 1];
assert(sm_nbytes > 0);

}

for ( i = 0; i < rank; i++)
{
obj.chunk.chunk_lengths[i] = sm_size[i];
}

}

for ( i=0; i<obj.nfilters; i++)
{

unsigned     aggression;     *//* the deflate level *//*

aggression = obj.filter[i].cd_values[0];
*//* set up for deflated data *//*
if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
return -1;
if(H5Pset_deflate(dcpl_id,aggression)<0)
return -1;
    }

    }
    return 0;
}*/

static int do_copy_objects_error(hid_t grp_in, hid_t grp_out, hid_t dcpl_id, hid_t gcpl_in, hid_t gcpl_out,
                                 hid_t f_space_id, hid_t dset_in, hid_t dset_out, hid_t ftype_id, hid_t wtype_id,
                                 hid_t type_in, hid_t type_out, named_dt_t *named_dt_head, void* buf, void* sm_buf){
    H5Gclose(grp_in);
    H5Gclose(grp_out);
    H5Pclose(dcpl_id);
    H5Pclose(gcpl_in);
    H5Pclose(gcpl_out);
    H5Sclose(f_space_id);
    H5Dclose(dset_in);
    H5Dclose(dset_out);
    H5Tclose(ftype_id);
    H5Tclose(wtype_id);
    H5Tclose(type_in);
    H5Tclose(type_out);
    named_datatype_free(&named_dt_head, 1);
    if (buf!=NULL)
        HDfree(buf);
    if (sm_buf!=NULL)
        HDfree(sm_buf);
    return -1;
}



int do_copy_objects(hid_t fidin,
                    hid_t fidout,
                    trav_table_t *travt,
                    pack_opt_t *options) /* repack options */
{
    hid_t    grp_in=-1;         /* group ID */
    hid_t    grp_out=-1;        /* group ID */
    hid_t    dset_in=-1;        /* read dataset ID */
    hid_t    dset_out=-1;       /* write dataset ID */
    hid_t    gcpl_in=-1;        /* group creation property list */
    hid_t    gcpl_out=-1;       /* group creation property list */
    hid_t    type_in=-1;        /* named type ID */
    hid_t    type_out=-1;       /* named type ID */
    hid_t    dcpl_id=-1;        /* dataset creation property list ID */
    hid_t    dcpl_out=-1;       /* dataset creation property list ID */
    hid_t    f_space_id=-1;     /* file space ID */
    hid_t    ftype_id=-1;       /* file type ID */
    hid_t    wtype_id=-1;       /* read/write type ID */
    named_dt_t *named_dt_head=NULL; /* Pointer to the stack of named datatypes copied */
    size_t   msize;             /* size of type */
    hsize_t  nelmts;            /* number of elements in dataset */
    int      rank;              /* rank of dataset */
    hsize_t  dims[H5S_MAX_RANK];/* dimensions of dataset */
    hsize_t  dsize_in;          /* input dataset size before filter */
    hsize_t  dsize_out;         /* output dataset size after filter */
    int      apply_s;           /* flag for apply filter to small dataset sizes */
    int      apply_f;           /* flag for apply filter to return error on H5Dcreate */
    void     *buf=NULL;         /* buffer for raw data */
    void     *sm_buf=NULL;      /* buffer for raw data */
    int      has_filter;        /* current object has a filter */
    int      req_filter;        /* there was a request for a filter */
    unsigned crt_order_flags;   /* group creation order flag */
    unsigned i;
    unsigned u;
    int      is_ref=0;
    htri_t   is_named;

    /*-------------------------------------------------------------------------
    * copy the suppplied object list
    *-------------------------------------------------------------------------
    */

    for ( i = 0; i < travt->nobjs; i++)
    {

        buf = NULL;
        switch ( travt->objs[i].type )
        {

            case H5TRAV_TYPE_UNKNOWN:
                assert(0);
                break;
                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_GROUP
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_GROUP:


                /* open input group */
                if ((grp_in = H5Gopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                /* get input group creation property list */
                if ((gcpl_in = H5Gget_create_plist(grp_in)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                /* query and set the group creation properties */
                if (H5Pget_link_creation_order(gcpl_in, &crt_order_flags) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                /* set up group creation property list */
                if ((gcpl_out = H5Pcreate(H5P_GROUP_CREATE)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if (H5Pset_link_creation_order(gcpl_out, crt_order_flags) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                /*-------------------------------------------------------------------------
                * the root is a special case, we get an ID for the root group
                * and copy its attributes using that ID
                *-------------------------------------------------------------------------
                */
                if(strcmp(travt->objs[i].name, "/") == 0)
                {
                    if ((grp_out = H5Gopen2(fidout, "/", H5P_DEFAULT)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                }

                else
                {

                    if (options->grp_compact>0 || options->grp_indexed>0)
                    {
                        if(H5Pset_link_phase_change(gcpl_out, (unsigned)options->grp_compact, (unsigned)options->grp_indexed) < 0)
                            return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    }

                    if((grp_out = H5Gcreate2(fidout, travt->objs[i].name, H5P_DEFAULT, gcpl_out, H5P_DEFAULT)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                }

                /*-------------------------------------------------------------------------
                * copy attrs
                *-------------------------------------------------------------------------
                */
                if(copy_attr(grp_in, grp_out, &named_dt_head, travt, options) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                if(H5Pclose(gcpl_out) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5Pclose(gcpl_in) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5Gclose(grp_out) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5Gclose(grp_in) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_DATASET
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_DATASET:

                has_filter = 0;
                req_filter = 0;

                /* check if global filters were requested */
                if ( options->n_filter_g )
                    req_filter = 1;

                /* check if filters were requested for individual objects */
                for (u = 0; u < options->op_tbl->nelems; u++) {

                    if (strcmp(travt->objs[i].name, options->op_tbl->objs[u].path) == 0) {
                        if (options->op_tbl->objs[u].filter->filtn > 0) {
                            req_filter = 1;
                        }
                    }
                }

                /* early detection of references */
                if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if((ftype_id = H5Dget_type(dset_in)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5T_REFERENCE == H5Tget_class(ftype_id))
                    is_ref = 1;

                /* Check if the datatype is committed */
                if((is_named = H5Tcommitted(ftype_id)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(is_named)
                    if((wtype_id = copy_named_datatype(ftype_id, fidout, &named_dt_head, travt, options)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                if(H5Tclose(ftype_id) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5Dclose(dset_in) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                /*-------------------------------------------------------------------------
                * check if we should use H5Ocopy or not
                * if there is a request for filters/layout, we read/write the object
                * otherwise we do a copy using H5Ocopy
                *-------------------------------------------------------------------------
                */
                if ( options->op_tbl->nelems  ||
                     options->all_filter == 1 ||
                     options->all_layout == 1 ||
                     is_ref ||
                     is_named)
                {

                    int      j;

                    if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((f_space_id = H5Dget_space(dset_in)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((ftype_id = H5Dget_type(dset_in)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((dcpl_id = H5Dget_create_plist(dset_in)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((dcpl_out = H5Pcopy(dcpl_id)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((rank = H5Sget_simple_extent_ndims(f_space_id)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    memset(dims, 0, sizeof dims);
                    if(H5Sget_simple_extent_dims(f_space_id, dims, NULL) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    nelmts = 1;
                    for ( j = 0; j < rank; j++)
                    {
                        nelmts *= dims[j];
                    }

                    /* wtype_id will have already been set if using a named dtype */
                    if(!is_named) {
                        if(options->use_native == 1)
                            wtype_id = 1;//h5tools_get_native_type(ftype_id);
                        else
                            wtype_id = H5Tcopy(ftype_id);
                    } /* end if */

                    if((msize = H5Tget_size(wtype_id)) == 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                    /*-------------------------------------------------------------------------
                    * check if the dataset creation property list has filters that
                    * are not registered in the current configuration
                    * 1) the external filters GZIP and SZIP might not be available
                    * 2) the internal filters might be turned off
                    *-------------------------------------------------------------------------
                    */
                    if (1)//h5tools_canreadf((travt->objs[i].name),dcpl_id)==1)
                    {
                        apply_s=1;
                        apply_f=1;

                        /*-------------------------------------------------------------------------
                        * references are a special case
                        * we cannot just copy the buffers, but instead we recreate the reference
                        * in a second traversal of the output file
                        *-------------------------------------------------------------------------
                        */
                        if (H5T_REFERENCE!=H5Tget_class(wtype_id))
                        {
                            /* get the storage size of the input dataset */
                            dsize_in=H5Dget_storage_size(dset_in);

                            /* check for small size datasets (less than 1k) except
                             * changing to COMPACT. For the reference, COMPACT is limited
                             * by size 64K by library.
                             */
                            if (options->layout_g != H5D_COMPACT)
                            {
                                if ( nelmts*msize < options->min_comp )
                                    apply_s=0;
                            }

                            /* apply the filter */
                        /*if (apply_s)
                        {
                            if (apply_filters(travt->objs[i].name,
                                rank,
                                dims,
                                msize,
                                dcpl_out,
                                options,
                                &has_filter) < 0)
                                goto error;
                        }*/

                            /*-------------------------------------------------------------------------
                            * create the output dataset;
                            * disable error checking in case the dataset cannot be created with the
                            * modified dcpl; in that case use the original instead
                            *-------------------------------------------------------------------------
                            */
                            H5E_BEGIN_TRY {
                                    dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_out, H5P_DEFAULT);
                                } H5E_END_TRY;

//                        if(dset_out == FAIL)
//                        {
//                            if(options->verbose)
//                                printf(" warning: could not create dataset <%s>. Applying original settings\n",
//                                travt->objs[i].name);
//
//                            if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
//                                goto error;
//                            apply_f = 0;
//                        }

                            /*-------------------------------------------------------------------------
                            * read/write
                            *-------------------------------------------------------------------------
                            */
                            if (nelmts)
                            {
                                size_t need = (size_t)(nelmts*msize);  /* bytes needed */
                                if ( need < 128*1024*1024 )
                                    buf = malloc(need);

                                if (buf != NULL )
                                {
                                    if (H5Dread(dset_in,wtype_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf) < 0)
                                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                                    if (H5Dwrite(dset_out,wtype_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf) < 0)
                                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                                }

                                else /* possibly not enough memory, read/write by hyperslabs */
                                {
                                    size_t        p_type_nbytes = msize; /*size of memory type */
                                    hsize_t       p_nelmts = nelmts;     /*total selected elmts */
                                    hsize_t       elmtno;                /*counter  */
                                    int           carry;                 /*counter carry value */
                                    unsigned int  vl_data = 0;           /*contains VL datatypes */

                                    /* stripmine info */
                                    hsize_t       sm_size[H5S_MAX_RANK]; /*stripmine size */
                                    hsize_t       sm_nbytes;             /*bytes per stripmine */
                                    hsize_t       sm_nelmts;             /*elements per stripmine*/
                                    hid_t         sm_space;              /*stripmine data space */

                                    /* hyperslab info */
                                    hsize_t       hs_offset[H5S_MAX_RANK];/*starting offset */
                                    hsize_t       hs_size[H5S_MAX_RANK];  /*size this pass */
                                    hsize_t       hs_nelmts;              /*elements in request */
                                    hsize_t       zero[8];                /*vector of zeros */
                                    int           k;

                                    /* check if we have VL data in the dataset's datatype */
                                    if (H5Tdetect_class(wtype_id, H5T_VLEN) == true)
                                        vl_data = true;

                                    /*
                                    * determine the strip mine size and allocate a buffer. The strip mine is
                                    * a hyperslab whose size is manageable.
                                    */
                                    sm_nbytes = p_type_nbytes;

                                    for (k = rank; k > 0; --k)
                                    {
                                        hsize_t size = (1024*1024) / sm_nbytes;
                                        if ( size == 0) /* datum size > H5TOOLS_BUFSIZE */
                                            size = 1;
                                        sm_size[k - 1] = min(dims[k - 1], size);
                                        sm_nbytes *= sm_size[k - 1];
                                        assert(sm_nbytes > 0);
                                    }
                                    sm_buf = malloc((size_t)sm_nbytes);

                                    sm_nelmts = sm_nbytes / p_type_nbytes;
                                    sm_space = H5Screate_simple(1, &sm_nelmts, NULL);

                                    /* the stripmine loop */
                                    memset(hs_offset, 0, sizeof hs_offset);
                                    memset(zero, 0, sizeof zero);

                                    for (elmtno = 0; elmtno < p_nelmts; elmtno += hs_nelmts)
                                    {
                                        /* calculate the hyperslab size */
                                        if (rank > 0)
                                        {
                                            for (k = 0, hs_nelmts = 1; k < rank; k++)
                                            {
                                                hs_size[k] = min(dims[k] - hs_offset[k], sm_size[k]);
                                                hs_nelmts *= hs_size[k];
                                            }

                                            if (H5Sselect_hyperslab(f_space_id, H5S_SELECT_SET, hs_offset, NULL, hs_size, NULL) < 0)
                                                return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                                            if (H5Sselect_hyperslab(sm_space, H5S_SELECT_SET, zero, NULL, &hs_nelmts, NULL) < 0)
                                                return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                                        }
                                        else
                                        {
                                            H5Sselect_all(f_space_id);
                                            H5Sselect_all(sm_space);
                                            hs_nelmts = 1;
                                        } /* rank */

                                        /* read/write */
                                        if (H5Dread(dset_in, wtype_id, sm_space, f_space_id, H5P_DEFAULT, sm_buf) < 0)
                                            return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                                        if (H5Dwrite(dset_out, wtype_id, sm_space, f_space_id, H5P_DEFAULT, sm_buf) < 0)
                                            return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                                        /* reclaim any VL memory, if necessary */
                                        if(vl_data)
                                            H5Dvlen_reclaim(wtype_id, sm_space, H5P_DEFAULT, sm_buf);

                                        /* calculate the next hyperslab offset */
                                        for (k = rank, carry = 1; k > 0 && carry; --k)
                                        {
                                            hs_offset[k - 1] += hs_size[k - 1];
                                            if (hs_offset[k - 1] == dims[k - 1])
                                                hs_offset[k - 1] = 0;
                                            else
                                                carry = 0;
                                        } /* k */
                                    } /* elmtno */

                                    H5Sclose(sm_space);
                                    /* free */
                                    if (sm_buf!=NULL)
                                    {
                                        HDfree(sm_buf);
                                        sm_buf=NULL;
                                    }
                                } /* hyperslab read */
                            }/*nelmts*/

                            /*-------------------------------------------------------------------------
                            * amount of compression used
                            *-------------------------------------------------------------------------
                            */
                            if (options->verbose)
                            {
                                double ratio=0;

                                /* only print the compression ration if there was a filter request */
                                if (apply_s && apply_f && req_filter)
                                {
                                    /* get the storage size of the output dataset */
                                    dsize_out=H5Dget_storage_size(dset_out);

                                    /* compression ratio = uncompressed size /  compressed size */
                                    if (dsize_out!=0)
                                        ratio = (double) dsize_in / (double) dsize_out;

                                    //print_dataset_info(dcpl_out,travt->objs[i].name,ratio,1);
                                }
                                else
                                    //print_dataset_info(dcpl_id,travt->objs[i].name,ratio,0);

                                    /* print a message that the filter was not applied
                                    (in case there was a filter)
                                    */
                                if ( has_filter && apply_s == 0 )
                                    printf(" <warning: filter not applied to %s. dataset smaller than %d bytes>\n",
                                           travt->objs[i].name,
                                           (int)options->min_comp);

                                if ( has_filter && apply_f == 0 )
                                    printf(" <warning: could not apply the filter to %s>\n",
                                           travt->objs[i].name);

                            } /* verbose */

                            /*-------------------------------------------------------------------------
                            * copy attrs
                            *-------------------------------------------------------------------------
                            */
                            if (copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                                return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                            /*close */
                            if (H5Dclose(dset_out) < 0)
                                return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                        }/*!H5T_REFERENCE*/
                    }/*h5tools_canreadf*/


                    /*-------------------------------------------------------------------------
                    * close
                    *-------------------------------------------------------------------------
                    */
                    if (H5Tclose(ftype_id) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if (H5Tclose(wtype_id) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if (H5Pclose(dcpl_id) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if (H5Pclose(dcpl_out) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if (H5Sclose(f_space_id) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if (H5Dclose(dset_in) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                }
                    /*-------------------------------------------------------------------------
                    * we do not have request for filter/chunking use H5Ocopy instead
                    *-------------------------------------------------------------------------
                    */
                else
                {
                    hid_t        pid;

                    /* create property to pass copy options */
                    if ( (pid = H5Pcreate(H5P_OBJECT_COPY)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                    /* set options for object copy */
                    if(H5Pset_copy_object(pid, H5O_COPY_WITHOUT_ATTR_FLAG) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                    /*-------------------------------------------------------------------------
                    * do the copy
                    *-------------------------------------------------------------------------
                    */

                    if(H5Ocopy(fidin,          /* Source file or group identifier */
                               travt->objs[i].name,       /* Name of the source object to be copied */
                               fidout,                    /* Destination file or group identifier  */
                               travt->objs[i].name,       /* Name of the destination object  */
                               pid,                       /* Properties which apply to the copy   */
                               H5P_DEFAULT) < 0)            /* Properties which apply to the new hard link */
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);


                    /* close property */
                    if(H5Pclose(pid) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                    /*-------------------------------------------------------------------------
                    * copy attrs manually
                    *-------------------------------------------------------------------------
                    */
                    if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if((dset_out = H5Dopen2(fidout, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if(H5Dclose(dset_in) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                    if(H5Dclose(dset_out) < 0)
                        return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                } /* end do we have request for filter/chunking */


                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_NAMED_DATATYPE
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_NAMED_DATATYPE:

                if((type_in = H5Topen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                /* Copy the datatype anonymously */
                if((type_out = copy_named_datatype(type_in, fidout, &named_dt_head,
                                                   travt, options)) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                /* Link in to group structure */
                if(H5Lcreate_hard(type_out, ".", fidout, travt->objs[i].name,
                                  H5P_DEFAULT, H5P_DEFAULT) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                /*-------------------------------------------------------------------------
                * copy attrs
                *-------------------------------------------------------------------------
                */
                if(copy_attr(type_in, type_out, &named_dt_head, travt, options) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

                if(H5Tclose(type_in) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);
                if(H5Tclose(type_out) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);



                break;


                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_LINK
                * H5TRAV_TYPE_UDLINK
                *
                * Only handles external links; H5Lcopy will fail for other UD link types
                * since we don't have creation or copy callbacks for them.
                *-------------------------------------------------------------------------
                */

            case H5TRAV_TYPE_LINK:
            case H5TRAV_TYPE_UDLINK:
            {

                if(H5Lcopy(fidin, travt->objs[i].name,fidout, travt->objs[i].name, H5P_DEFAULT, H5P_DEFAULT) < 0)
                    return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);

            }
                break;

            default:
                return do_copy_objects_error(grp_in, grp_out, dcpl_id, gcpl_in, gcpl_out, f_space_id, dset_in, dset_out, ftype_id, wtype_id, type_in, type_out, named_dt_head, buf, sm_buf);
        } /* switch */

        /* free */
        if (buf!=NULL)
        {
            HDfree(buf);
            buf=NULL;
        }

    } /* i */

    /* Finalize (link) the stack of named datatypes (if any) */
    named_datatype_free(&named_dt_head, 0);

    return 0;
}

int copy_objects_error(hid_t fapl, hid_t fcpl, hid_t fidin, hid_t fidout, trav_table_t * travt){
    H5E_BEGIN_TRY
        {
            H5Pclose(fapl);
            H5Pclose(fcpl);
            H5Fclose(fidin);
            H5Fclose(fidout);
        } H5E_END_TRY;
    if(travt)
        trav_table_free(travt);
    return -1;
}

void init_packobject(pack_info_t *obj) {
    int j, k;

    strcpy(obj->path, "\0");
    for (j = 0; j < H5_REPACK_MAX_NFILTERS; j++) {
        obj->filter[j].filtn = -1;
        for (k = 0; k < CD_VALUES; k++)
            obj->filter[j].cd_values[k] = 0;
    }
    obj->chunk.rank = -1;
    obj->refobj_id = -1;
    obj->layout = H5D_LAYOUT_ERROR;
    obj->nfilters = 0;
}

pack_opt_t * createDefaultOptions(){
    pack_opt_t * options = new pack_opt_t;
    options->min_comp = 1024;

    for (int n = 0; n < H5_REPACK_MAX_NFILTERS; n++)
    {
        options->filter_g[n].filtn  = -1;
        options->filter_g[n].cd_nelmts  = 0;
        for (int k = 0; k < CD_VALUES; k++)
            options->filter_g[n].cd_values[k] = 0;
    }

    pack_opttbl_t *table = new pack_opttbl_t;
    table->size =30;
    table->nelems = 0;
    table->objs = new pack_info_t;

    for (int i = 0; i < table->size; i++)
       //init_packobject(&table->objs[i]);

    options->op_tbl = table;

    filter_info_t * filt = new filter_info_t;
    filt->cd_nelmts = 1;
    filt->filtn = H5Z_FILTER_DEFLATE;
    filt->cd_values[0] = 9;
    options->filter_g[0] = *filt;
    options->n_filter_g++;

    return options;
}

int h5repack::copy_objects(H5File fileIn,
                 const char* fnameout)
{
    hid_t         fidin;
    hid_t         fidout = -1;
    trav_table_t  *travt = NULL;
    hsize_t       ub_size = 0;        /* size of user block */
    hid_t         fcpl = H5P_DEFAULT; /* file creation property list ID */
    hid_t         fapl = H5P_DEFAULT; /* file access property list ID */

    pack_opt_t * options = createDefaultOptions();

    /*-------------------------------------------------------------------------
    * open input file
    *-------------------------------------------------------------------------
    */
    fidin = fileIn.getId();

    /* get user block size */
    {
        hid_t fcpl_in; /* file creation property list ID for input file */

        if((fcpl_in = H5Fget_create_plist(fidin)) < 0)
        {
            return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
        }

        if(H5Pget_userblock(fcpl_in, &ub_size) < 0)
        {
            return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
        }

        if(H5Pclose(fcpl_in) < 0)
        {
            return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
        }
    }

    /*-------------------------------------------------------------------------
    * create the output file
    *-------------------------------------------------------------------------
    */


    if((fidout = H5Fcreate(fnameout,H5F_ACC_TRUNC, fcpl, fapl)) < 0)
    {
        return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
    }


    /*-------------------------------------------------------------------------
    * get list of objects
    *-------------------------------------------------------------------------
    */

    /* init table */
    trav_table_init(&travt);

    /* get the list of objects in the file */
    if(h5trav_gettable(fidin, travt) < 0)
        return copy_objects_error(fapl, fcpl, fidin, fidout, travt);

    /*-------------------------------------------------------------------------
    * do the copy
    *-------------------------------------------------------------------------
    */
    if(do_copy_objects(fidin, fidout, travt, options) < 0)
    {
        return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
    } /* end if */


    /* close */

    if(fapl > 0)
        H5Pclose(fapl);

    if(fcpl > 0)
        H5Pclose(fcpl);

    H5Fclose(fidin);
    H5Fclose(fidout);

    trav_table_free(travt);
    travt = NULL;

    if( ub_size > 0 && options->ublock_size == 0 )
    {
        if ( copy_user_block(fileIn.getFileName().c_str(), fnameout, ub_size) < 0 )
        {
            return copy_objects_error(fapl, fcpl, fidin, fidout, travt);
        }
    }
    return 0;
}