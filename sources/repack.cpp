#include "../headers/repack.hpp"
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>


using namespace std;
int         opt_ind = 1;
const hsize_t H5TOOLS_BUFSIZE = 33554432;  /* 32 MB */

void error_msg(string errorMessage){

}

void error_msg(string errorMessage, string cuco){

}

hid_t h5tools_get_native_type(hid_t type_in){
    return H5Tcopy(type_in);
}

static struct long_options l_opts[] = {
        { "help", no_arg, 'h' },
        { "hel", no_arg, 'h'},
        { "he", no_arg, 'h'},
        { "version", no_arg, 'V' },
        { "version", no_arg, 'V' },
        { "versio", no_arg, 'V' },
        { "versi", no_arg, 'V' },
        { "vers", no_arg, 'V' },
        { "status", no_arg, 's' },
        { "statu", no_arg, 's' },
        { "stat", no_arg, 's' },
        { "sta", no_arg, 's' },
        { "st", no_arg, 's' },
        { "image", no_arg, 'm' },
        { "imag", no_arg, 'm' },
        { "ima", no_arg, 'm' },
        { "im", no_arg, 'm' },
        { "filesize", no_arg, 'z' },
        { "filesiz", no_arg, 'z' },
        { "filesi", no_arg, 'z' },
        { "files", no_arg, 'z' },
        { "file", no_arg, 'z' },
        { "fil", no_arg, 'z' },
        { "fi", no_arg, 'z' },
        { "increment", optional_arg, 'i' },
        { "incremen", optional_arg, 'i' },
        { "increme", optional_arg, 'i' },
        { "increm", optional_arg, 'i' },
        { "incre", optional_arg, 'i' },
        { "incr", optional_arg, 'i' },
        { "inc", optional_arg, 'i' },
        { "in", optional_arg, 'i' },
        { NULL, 0, '\0' }
} ;

static void
trav_addr_add(trav_addr_t *visited, haddr_t addr, const char *path)
{
    size_t idx;         /* Index of address to use */

    /* Allocate space if necessary */
    if(visited->nused == visited->nalloc) {
        visited->nalloc = MAX(1, visited->nalloc * 2);;
        visited->objs = (bupBup*)realloc(visited->objs, visited->nalloc * sizeof(visited->objs[0]));
    } /* end if */

    /* Append it */
    idx = visited->nused++;
    visited->objs[idx].addr = addr;
    visited->objs[idx].path = strdup(path);
} /* end trav_addr_add() */

static const char *
trav_addr_visited(trav_addr_t *visited, haddr_t addr)
{
    size_t u;           /* Local index variable */

    /* Look for address */
    for(u = 0; u < visited->nused; u++)
        /* Check for address already in array */
        if(visited->objs[u].addr == addr)
            return(visited->objs[u].path);

    /* Didn't find address */
    return(NULL);
} /* end trav_addr_visited() */

static herr_t
traverse_cb(hid_t loc_id, const char *path, const H5L_info_t *linfo,
            void *_udata)
{
    trav_ud_traverse_t *udata = (trav_ud_traverse_t *)_udata;     /* User data */
    char *new_name = NULL;
    const char *full_name;
    const char *already_visited = NULL; /* Whether the link/object was already visited */

    /* Create the full path name for the link */
    if(udata->is_absolute) {
        size_t base_len = strlen(udata->base_grp_name);
        size_t add_slash = base_len ? ((udata->base_grp_name)[base_len-1] != '/') : 1;

        if(NULL == (new_name = (char*)malloc(base_len + add_slash + strlen(path) + 1)))
            return(H5_ITER_ERROR);
        strcpy(new_name, udata->base_grp_name);
        if (add_slash)
            new_name[base_len] = '/';
        strcpy(new_name + base_len + add_slash, path);
        full_name = new_name;
    } /* end if */
    else
        full_name = path;

    /* Perform the correct action for different types of links */
    if(linfo->type == H5L_TYPE_HARD) {
        H5O_info_t oinfo;

        /* Get information about the object */
        if(H5Oget_info_by_name(loc_id, path, &oinfo, H5P_DEFAULT) < 0) {
            if(new_name)
                free(new_name);
            return(H5_ITER_ERROR);
        }

        /* If the object has multiple links, add it to the list of addresses
         *  already visited, if it isn't there already
         */
        if(oinfo.rc > 1)
            if(NULL == (already_visited = trav_addr_visited(udata->seen, oinfo.addr)))
                trav_addr_add(udata->seen, oinfo.addr, full_name);

        /* Make 'visit object' callback */
        if(udata->visitor->visit_obj)
            if((*udata->visitor->visit_obj)(full_name, &oinfo, already_visited, udata->visitor->udata) < 0) {
                if(new_name)
                    free(new_name);
                return(H5_ITER_ERROR);
            }
    } /* end if */
    else {
        /* Make 'visit link' callback */
        if(udata->visitor->visit_lnk)
            if((*udata->visitor->visit_lnk)(full_name, linfo, udata->visitor->udata) < 0) {
                if(new_name)
                    free(new_name);
                return(H5_ITER_ERROR);
            }
    } /* end else */

    if(new_name)
        free(new_name);

    return(H5_ITER_CONT);
} /* end traverse_cb() */


static int
traverse(hid_t file_id, const char *grp_name, hbool_t visit_start,
         hbool_t recurse, const trav_visitor_t *visitor)
{
    H5O_info_t  oinfo;          /* Object info for starting group */

    /* Get info for starting object */
    if(H5Oget_info_by_name(file_id, grp_name, &oinfo, H5P_DEFAULT) < 0)
        return -1;

    /* Visit the starting object */
    if(visit_start && visitor->visit_obj)
        (*visitor->visit_obj)(grp_name, &oinfo, NULL, visitor->udata);

    /* Go visiting, if the object is a group */
    if(oinfo.type == H5O_TYPE_GROUP) {
        trav_addr_t seen;           /* List of addresses seen */
        trav_ud_traverse_t udata;   /* User data for iteration callback */

        /* Init addresses seen */
        seen.nused = seen.nalloc = 0;
        seen.objs = NULL;

        /* Check for multiple links to top group */
        if(oinfo.rc > 1)
            trav_addr_add(&seen, oinfo.addr, grp_name);

        /* Set up user data structure */
        udata.seen = &seen;
        udata.visitor = visitor;
        udata.is_absolute = (*grp_name == '/');
        udata.base_grp_name = grp_name;

        /* Check for iteration of links vs. visiting all links recursively */
        if(recurse) {
            /* Visit all links in group, recursively */
            if(H5Lvisit_by_name(file_id, grp_name, H5_INDEX_NAME, H5_ITER_INC, traverse_cb, &udata, H5P_DEFAULT) < 0)
                return -1;
        } /* end if */
        else {
            /* Iterate over links in group */
            if(H5Literate_by_name(file_id, grp_name, H5_INDEX_NAME, H5_ITER_INC, NULL, traverse_cb, &udata, H5P_DEFAULT) < 0)
                return -1;
        } /* end else */

        /* Free visited addresses table */
        if(seen.objs) {
            size_t u;       /* Local index variable */

            /* Free paths to objects */
            for(u = 0; u < seen.nused; u++)
                free(seen.objs[u].path);
            free(seen.objs);
        } /* end if */
    } /* end if */

    return 0;
}

int options_table_free( pack_opttbl_t *table )
{
    free(table->objs);
    free(table);
    return 0;
}

static void
trav_table_add(trav_table_t *table,
               const char *path,
               const H5O_info_t *oinfo)
{
    size_t newObj;

    if(table->nobjs == table->size) {
        table->size = MAX(1, table->size * 2);
        table->objs = (trav_obj_t*)realloc(table->objs, table->size * sizeof(trav_obj_t));
    } /* end if */

    newObj = table->nobjs++;
    table->objs[newObj].objno = oinfo ? oinfo->addr : HADDR_UNDEF;
    table->objs[newObj].flags[0] = table->objs[newObj].flags[1] = 0;
    table->objs[newObj].name = (char *)strdup(path);
    table->objs[newObj].type = oinfo ? (h5trav_type_t)oinfo->type : H5TRAV_TYPE_LINK;
    table->objs[newObj].nlinks = 0;
    table->objs[newObj].sizelinks = 0;
    table->objs[newObj].links = NULL;
}


static void
trav_table_addlink(trav_table_t *table, haddr_t objno, const char *path)
{
    size_t i;           /* Local index variable */

    for(i = 0; i < table->nobjs; i++) {
        if(table->objs[i].objno == objno) {
            size_t n;

/* already inserted? */
            if(strcmp(table->objs[i].name, path) == 0)
                return;

/* allocate space if necessary */
            if(table->objs[i].nlinks == (unsigned)table->objs[i].sizelinks) {
                table->objs[i].sizelinks = MAX(1, table->objs[i].sizelinks * 2);
                table->objs[i].links = (trav_link_t*)realloc(table->objs[i].links, table->objs[i].sizelinks * sizeof(trav_link_t));
            } /* end if */

/* insert it */

            n = table->objs[i].nlinks++;
            table->objs[i].links[n].new_name = (char *)strdup(path);

            return;
        } /* end for */
    } /* end for */

    assert(0 && "object not in table?!?");
}

void init_packobject(pack_info_t *obj)
{
    int j, k;

    strcpy(obj->path,"\0");
    for ( j=0; j<H5_REPACK_MAX_NFILTERS; j++)
    {
        obj->filter[j].filtn        = -1;
        for ( k=0; k<CD_VALUES; k++)
            obj->filter[j].cd_values[k] = 0;
    }
    obj->chunk.rank = -1;
    obj->refobj_id = -1;
    obj->layout = H5D_LAYOUT_ERROR;
    obj->nfilters = 0;
}


static int
trav_table_visit_obj(const char *path, const H5O_info_t *oinfo,
                     const char *already_visited, void *udata)
{
    trav_table_t *table = (trav_table_t *)udata;

    /* Check if we've already seen this object */
    if(NULL == already_visited)
        /* add object to table */
        trav_table_add(table, path, oinfo);
    else
        /* Add alias for object to table */
        trav_table_addlink(table, oinfo->addr, path);

    return(0);
} /* end trav_table_visit_obj() */


static int
trav_table_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata)
{
/* Add the link to the 'table' struct */
trav_table_add((trav_table_t *)udata, path, NULL);

return(0);
} /* end trav_table_visit_lnk() */

int
h5trav_gettable(hid_t fid, trav_table_t *table)
{
    trav_visitor_t table_visitor;       /* Visitor structure for trav_table_t's */

    /* Init visitor structure */
    table_visitor.visit_obj = trav_table_visit_obj;
    table_visitor.visit_lnk = trav_table_visit_lnk;
    table_visitor.udata = table;

    /* Traverse all objects in the file, visiting each object & link */
    if(traverse(fid, "/", true, true, &table_visitor) < 0)
        return -1;
    return 0;
}

void trav_table_init(trav_table_t **tbl)
{
    trav_table_t* table = (trav_table_t*) malloc(sizeof(trav_table_t));

    table->size = 0;
    table->nobjs = 0;
    table->objs = NULL;

    *tbl = table;
}

static const char*
MapIdToName(hid_t refobj_id, trav_table_t *travt)
{
    unsigned int u;
    const char* ret = NULL;

    /* linear search */
    for(u = 0; u < travt->nobjs; u++) {
        if(travt->objs[u].type == H5O_TYPE_DATASET ||
           travt->objs[u].type == H5O_TYPE_GROUP ||
           travt->objs[u].type == H5O_TYPE_NAMED_DATATYPE) {
            H5O_info_t   ref_oinfo;     /* Stat for the refobj id */

            /* obtain information to identify the referenced object uniquely */
            if(H5Oget_info(refobj_id, &ref_oinfo) < 0)
                goto out;

            if(ref_oinfo.addr == travt->objs[u].objno) {
                ret = travt->objs[u].name;
                goto out;
            } /* end if */
        }  /* end if */
    } /* u */

    out:
    return ret;
}

void trav_table_free( trav_table_t *table )
{
    if(table->objs) {
        unsigned int i;

        for(i = 0; i < table->nobjs; i++) {
            free(table->objs[i].name );
            if(table->objs[i].nlinks) {
                unsigned int j;

                for(j = 0; j < table->objs[i].nlinks; j++)
                    free(table->objs[i].links[j].new_name);

                free(table->objs[i].links);
            } /* end if */
        } /* end for */
        free(table->objs);
    } /* end if */
    free(table);
}

int h5tools_canreadf(const char* name, /* object name, serves also as boolean print */
                     hid_t dcpl_id)    /* dataset creation property list */
{

    int          nfilters;       /* number of filters */
    H5Z_filter_t filtn;          /* filter identification number */
    int          i;              /* index */
    int          have_deflate=0; /* assume initially we do not have filters */
    int          have_szip=0;
    int          have_shuffle=0;
    int          have_fletcher=0;

#ifdef H5_HAVE_FILTER_DEFLATE
    have_deflate=1;
#endif
#ifdef H5_HAVE_FILTER_SZIP
    have_szip=1;
#endif
#ifdef H5_HAVE_FILTER_SHUFFLE
    have_shuffle=1;
#endif
#ifdef H5_HAVE_FILTER_FLETCHER32
    have_fletcher=1;
#endif

    /* get information about filters */
    if ((nfilters = H5Pget_nfilters(dcpl_id))<0)
        return -1;

    /* if we do not have filters, we can read the dataset safely */
    if (!nfilters)
        return 1;

    /* check availability of filters */
    for (i=0; i<nfilters; i++) {
        if ((filtn = H5Pget_filter(dcpl_id, (unsigned) i, 0, 0, 0, 0, 0, 0)) < 0)
            return -1;
    }
    return 1;
}

static int aux_find_obj(const char* name,          /* object name from traverse list */
                 pack_opt_t *options,       /* repack options */
                 pack_info_t *obj /*OUT*/)  /* info about object to filter */
{
    return -1;
}

static
int aux_assign_obj(const char* name,            /* object name from traverse list */
                   pack_opt_t *options,         /* repack options */
                   pack_info_t *obj /*OUT*/)    /* info about object to filter */
{

    int  idx, i;
    pack_info_t tmp;

    init_packobject(&tmp);

    idx = aux_find_obj(name,options,&tmp);

    if (options->all_filter)
    {
        int k;

        /* assign the global filters */
        tmp.nfilters=options->n_filter_g;
        for ( k = 0; k < options->n_filter_g; k++)
            tmp.filter[k]=options->filter_g[k];
    }
    if (options->all_layout)
    {
        /* assign the global layout info to the OBJ info */
        tmp.layout=options->layout_g;
        switch (options->layout_g)
        {
            case H5D_CHUNKED:
                tmp.chunk.rank=options->chunk_g.rank;
                for ( i=0; i<tmp.chunk.rank; i++)
                    tmp.chunk.chunk_lengths[i]=options->chunk_g.chunk_lengths[i];
                break;
            case H5D_LAYOUT_ERROR:
            case H5D_COMPACT:
            case H5D_CONTIGUOUS:
            case H5D_NLAYOUTS:
                break;
            default:
                break;
        }/*switch*/
    }

    *obj = tmp;
    return 1;

}


obj_list_t* parse_filter(const char *str,
                         int *n_objs,
                         filter_info_t *filt,
                         pack_opt_t *options,
                         int *is_glb)
{
    unsigned    i, u;
    char        c;
    size_t      len=strlen(str);
    int         j, m, n, k, l, end_obj=-1, no_param=0;
    char        sobj[MAX_NC_NAME];
    char        scomp[10];
    char        stype[5];
    char        smask[3];
    obj_list_t* obj_list=NULL;
    unsigned    pixels_per_block;


    /* initialize compression  info */
    memset(filt,0,sizeof(filter_info_t));
    *is_glb = 0;

    /* check for the end of object list and number of objects */
    for ( i = 0, n = 0; i < len; i++)
    {
        c = str[i];
        if ( c==':' )
        {
            end_obj=i;
        }
        if ( c==',' )
        {
            n++;
        }
    }

    if (end_obj==-1) /* missing : */
    {
        /* apply to all objects */
        options->all_filter=1;
        *is_glb = 1;
    }

    n++;
    obj_list = (obj_list_t*) malloc(n*sizeof(obj_list_t));
    if (obj_list==NULL)
    {
        error_msg("could not allocate object list\n");
        return NULL;
    }
    *n_objs=n;

    /* get object list */
    for ( j = 0, k = 0, n = 0; j < end_obj; j++, k++)
    {
        c = str[j];
        sobj[k] = c;
        if ( c==',' || j==end_obj-1)
        {
            if ( c==',') sobj[k]='\0'; else sobj[k+1]='\0';
            strcpy(obj_list[n].obj,sobj);
            memset(sobj,0,sizeof(sobj));
            n++;
            k=-1;
        }
    }
    /* nothing after : */
    if (end_obj+1==(int)len)
    {
        if (obj_list) free(obj_list);
        error_msg("input Error: Invalid compression type in <%s>\n",str);
        exit(EXIT_FAILURE);
    }


    /* get filter additional parameters */
    m=0;
    for ( i=end_obj+1, k=0, j=0; i<len; i++,k++)
    {
        c = str[i];
        scomp[k]=c;
        if ( c=='=' || i==len-1)
        {
            if ( c=='=') /*one more parameter */
            {
                scomp[k]='\0';     /*cut space */

                    /* here we could have 1 or 2 digits  */
                for ( m=0,u=i+1; u<len; u++,m++)
                {
                    c = str[u];
                    if (!isdigit(c)){
                        if (obj_list) free(obj_list);
                        error_msg("compression parameter is not a digit in <%s>\n",str);
                        exit(EXIT_FAILURE);
                    }
                    stype[m]=c;
                } /* u */

                stype[m]='\0';

                filt->cd_values[j++]=atoi(stype);
                i+=m; /* jump */
            }
            else if (i==len-1)
            { /*no more parameters */
                scomp[k+1]='\0';
                no_param=1;
            }

            if (strcmp(scomp,"GZIP")==0)
            {
                filt->filtn=H5Z_FILTER_DEFLATE;
                filt->cd_nelmts = 1;
                if (no_param)
                { /*no more parameters, GZIP must have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("missing compression parameter in <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }
        }
    } /*i*/

    switch (filt->filtn)
    {
        case H5Z_FILTER_DEFLATE:
            if (filt->cd_values[0]>9 )
            {
                if (obj_list) free(obj_list);
                error_msg("invalid compression parameter in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    };

    return obj_list;
}

int h5repack_end  (pack_opt_t *options)
{
    return options_table_free(options->op_tbl);
}

int h5repack_addfilter(const char* str,
                       pack_opt_t *options)
{
    obj_list_t      *obj_list=NULL; /* one object list for the -f and -l option entry */
    filter_info_t   filter;         /* filter info for the current -f option entry */
    int             n_objs;         /* number of objects in the current -f or -l option entry */
    int             is_glb;         /* is the filter global */



    /* parse the -f option */
    if(NULL == (obj_list = parse_filter(str, &n_objs, &filter, options, &is_glb)))
        return -1;

    /* if it applies to all objects */
    if(is_glb)
    {
        int n;

        n = options->n_filter_g++; /* increase # of global filters */

        if(options->n_filter_g > H5_REPACK_MAX_NFILTERS)
        {
            error_msg("maximum number of filters exceeded for <%s>\n");
            free(obj_list);
            return -1;
        }

        options->filter_g[n] = filter;
    }
    free(obj_list);
    return 0;
}


hid_t copy_named_datatype(hid_t type_in, hid_t fidout, named_dt_t **named_dt_head_p, trav_table_t *travt, pack_opt_t *options)
{
    named_dt_t  *dt = *named_dt_head_p; /* Stack pointer */
    named_dt_t  *dt_ret = NULL;     /* Datatype to return */
    H5O_info_t  oinfo;              /* Object info of input dtype */
    hid_t       ret_value = -1;     /* The identifier of the named dtype in the out file */

    if(H5Oget_info(type_in, &oinfo) < 0)
        goto error;

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
                if(NULL == (dt = (named_dt_t *) malloc(sizeof(named_dt_t))))
                    goto error;
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
        if(NULL == (dt_ret = (named_dt_t *) malloc(sizeof(named_dt_t))))
            goto error;
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
            dt_ret->id_out = h5tools_get_native_type(type_in);
        else
            dt_ret->id_out = H5Tcopy(type_in);
        if(dt_ret->id_out < 0)
            goto error;
        if(H5Tcommit_anon(fidout, dt_ret->id_out, H5P_DEFAULT, H5P_DEFAULT) < 0)
            goto error;
    } /* end if */

    /* Set return value */
    ret_value = dt_ret->id_out;

    /* Increment the ref count on id_out, because the calling function will try
     * to close it */
    if(H5Iinc_ref(ret_value) < 0)
        goto error;

    return(ret_value);

    error:
    return(-1);
} /* end copy_named_datatype */


int named_datatype_free(named_dt_t **named_dt_head_p, int ignore_err)
{
    named_dt_t *dt = *named_dt_head_p;

    while(dt)
    {
        /* Pop the datatype off the stack and free it */
        if(H5Tclose(dt->id_out) < 0 && !ignore_err)
            goto error;
        dt = dt->next;
        free(*named_dt_head_p);
        *named_dt_head_p = dt;
    } /* end while */

    return 0;

    error:
    return -1;
} /* end named_datatype_free */



int apply_filters(const char* name,    /* object name from traverse list */
                  int rank,            /* rank of dataset */
                  hsize_t *dims,       /* dimensions of dataset */
                  size_t msize,        /* size of type */
                  hid_t dcpl_id,       /* dataset creation property list */
                  pack_opt_t *options, /* repack options */
                  int *has_filter)     /* (OUT) object NAME has a filter */


{
    int          nfilters;       /* number of filters in DCPL */
    hsize_t      chsize[64];     /* chunk size in elements */
    H5D_layout_t layout;
    int          i;
    pack_info_t  obj;

    *has_filter = 0;

    if (rank==0) /* scalar dataset, do not apply */
        return 0;

    init_packobject(&obj);

    if (aux_assign_obj(name,options,&obj)==0)
        return 0;

    /* get information about input filters */
    if ((nfilters = H5Pget_nfilters(dcpl_id))<0)
        return -1;

    if (nfilters && obj.nfilters )
    {
        *has_filter = 1;
        if (H5Premove_filter(dcpl_id,H5Z_FILTER_ALL)<0)
            return -1;
    }

    if (obj.layout == -1 )
    {
        if ((layout = H5Pget_layout(dcpl_id))<0)
            return -1;

        if (layout == H5D_CHUNKED)
        {
            if ((rank = H5Pget_chunk(dcpl_id,NELMTS(chsize),chsize/*out*/))<0)
                return -1;
            obj.layout = H5D_CHUNKED;
            obj.chunk.rank = rank;
            for ( i = 0; i < rank; i++)
                obj.chunk.chunk_lengths[i] = chsize[i];
        }
    }

    if (obj.nfilters)
    {

        if (obj.layout==-1)
        {

            /* stripmine info */
            hsize_t sm_size[H5S_MAX_RANK]; /*stripmine size */
            hsize_t sm_nbytes;             /*bytes per stripmine */

            obj.chunk.rank = rank;

            sm_nbytes = msize;
            for ( i = rank; i > 0; --i)
            {
                hsize_t size = H5TOOLS_BUFSIZE / sm_nbytes;
                if ( size == 0) /* datum size > H5TOOLS_BUFSIZE */
                    size = 1;
                sm_size[i - 1] = min(dims[i - 1], size);
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
            switch (obj.filter[i].filtn)
            {
                default:
                    break;
                case H5Z_FILTER_DEFLATE:
                {
                    unsigned     aggression;     /* the deflate level */

                    aggression = obj.filter[i].cd_values[0];
                    /* set up for deflated data */
                    if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                        return -1;
                    if(H5Pset_deflate(dcpl_id,aggression)<0)
                        return -1;
                }
                break;
            } /* switch */
        }/*i*/

    }

    if (obj.layout>=0)
    {
        /* a layout was defined */
        if (H5Pset_layout(dcpl_id, obj.layout)<0)
            return -1;

        if (H5D_CHUNKED == obj.layout)
        {
            if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                return -1;
        }
        else if (H5D_COMPACT == obj.layout)
        {
            if (H5Pset_alloc_time(dcpl_id, H5D_ALLOC_TIME_EARLY)<0)
                return -1;
        }
            /* remove filters for the H5D_CONTIGUOUS case */
        else if (H5D_CONTIGUOUS == obj.layout)
        {
            if (H5Premove_filter(dcpl_id,H5Z_FILTER_ALL)<0)
                return -1;
        }

    }

    return 0;
}


static int copy_refs_attr(hid_t loc_in,
                          hid_t loc_out,
                          pack_opt_t *options,
                          trav_table_t *travt,
                          hid_t fidout         /* for saving references */
)
{
    hid_t      attr_id = -1;      /* attr ID */
    hid_t      attr_out = -1;     /* attr ID */
    hid_t      space_id = -1;     /* space ID */
    hid_t      ftype_id = -1;     /* file data type ID */
    hid_t      mtype_id = -1;     /* memory data type ID */
    size_t     msize;             /* memory size of type */
    hsize_t    nelmts;            /* number of elements in dataset */
    int        rank;              /* rank of dataset */
    hsize_t    dims[H5S_MAX_RANK];/* dimensions of dataset */
    char       name[255];
    H5O_info_t oinfo;           /* Object info */
    int        j;
    unsigned   u;

    if(H5Oget_info(loc_in, &oinfo) < 0)
        goto error;

    for(u = 0; u < (unsigned)oinfo.num_attrs; u++)
    {
        /*-------------------------------------------------------------------------
        * open
        *-------------------------------------------------------------------------
        */
        /* open attribute */
        if((attr_id = H5Aopen_by_idx(loc_in, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t)u, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            goto error;

        /* get name */
        if(H5Aget_name(attr_id, 255, name) < 0)
            goto error;

        /* get the file datatype  */
        if((ftype_id = H5Aget_type(attr_id)) < 0)
            goto error;

        /* get the dataspace handle  */
        if((space_id = H5Aget_space(attr_id)) < 0)
            goto error;

        /* get dimensions  */
        if((rank = H5Sget_simple_extent_dims(space_id, dims, NULL)) < 0)
            goto error;


        /*-------------------------------------------------------------------------
        * elements
        *-------------------------------------------------------------------------
        */
        nelmts = 1;
        for(j = 0; j < rank; j++)
            nelmts *= dims[j];

        if((mtype_id = h5tools_get_native_type(ftype_id)) < 0)
            goto error;

        if((msize = H5Tget_size(mtype_id)) == 0)
            goto error;


        /*-------------------------------------------------------------------------
        * object references are a special case
        * we cannot just copy the buffers, but instead we recreate the reference
        *-------------------------------------------------------------------------
        */
        if(H5Tequal(mtype_id, H5T_STD_REF_OBJ))
        {
            hid_t       refobj_id;
            hobj_ref_t  *refbuf = NULL;
            unsigned    k;
            const char* refname;
            hobj_ref_t  *buf = NULL;

            /*-------------------------------------------------------------------------
            * read input to memory
            *-------------------------------------------------------------------------
            */

            if (nelmts)
            {
                buf = (hobj_ref_t *)malloc((unsigned)(nelmts * msize));
                if(buf == NULL)
                {
                    printf("cannot read into memory\n");
                    goto error;
                } /* end if */
                if(H5Aread(attr_id, mtype_id, buf) < 0)
                    goto error;

                refbuf = (hobj_ref_t *)calloc((unsigned)nelmts, msize);
                if(refbuf == NULL)
                {
                    printf( "cannot allocate memory\n" );
                    goto error;
                } /* end if */

                for(k = 0; k < nelmts; k++)
                {
                    H5E_BEGIN_TRY
                        {
                            if((refobj_id = H5Rdereference1(attr_id, H5R_OBJECT, &buf[k])) < 0)
                                goto error;
                        } H5E_END_TRY;

                    /* get the name. a valid name could only occur in the
                     * second traversal of the file
                     */
                    if((refname = MapIdToName(refobj_id, travt)) != NULL)
                    {
                        /* create the reference */
                        if(H5Rcreate(&refbuf[k], fidout, refname, H5R_OBJECT, -1) < 0)
                            goto error;
                    }
                    H5Oclose(refobj_id);
                } /* k */
            } /*nelmts*/

            /*-------------------------------------------------------------------------
            * copy
            *-------------------------------------------------------------------------
            */
            if((attr_out = H5Acreate2(loc_out, name, ftype_id, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                goto error;
            if(nelmts)
                if(H5Awrite(attr_out, mtype_id, refbuf) < 0)
                    goto error;

            if(H5Aclose(attr_out) < 0)
                goto error;

            if(refbuf)
                free(refbuf);
            if(buf)
                free(buf);
        }/*H5T_STD_REF_OBJ*/

            /*-------------------------------------------------------------------------
            * dataset region references
            *-------------------------------------------------------------------------
            */
        else if(H5Tequal(mtype_id, H5T_STD_REF_DSETREG))
        {
            hid_t            refobj_id;
            hdset_reg_ref_t  *refbuf = NULL; /* input buffer for region references */
            hdset_reg_ref_t  *buf = NULL;    /* output buffer */
            const char*      refname;
            unsigned         k;

            /*-------------------------------------------------------------------------
            * read input to memory
            *-------------------------------------------------------------------------
            */
            if(nelmts)
            {
                buf = (hdset_reg_ref_t *)malloc((unsigned)(nelmts * msize));
                if(buf == NULL)
                {
                    printf( "cannot read into memory\n" );
                    goto error;
                } /* end if */
                if(H5Aread(attr_id, mtype_id, buf) < 0)
                    goto error;

                /*-------------------------------------------------------------------------
                * create output
                *-------------------------------------------------------------------------
                */
                refbuf = (hdset_reg_ref_t *)calloc(sizeof(hdset_reg_ref_t), (size_t)nelmts); /*init to zero */
                if(refbuf == NULL)
                {
                    printf( "cannot allocate memory\n" );
                    goto error;
                } /* end if */

                for(k = 0; k < nelmts; k++)
                {
                    H5E_BEGIN_TRY
                        {
                            if((refobj_id = H5Rdereference1(attr_id, H5R_DATASET_REGION, &buf[k])) < 0)
                                continue;
                        } H5E_END_TRY;

                    /* get the name. a valid name could only occur in the
                     * second traversal of the file
                     */
                    if((refname = MapIdToName(refobj_id, travt)) != NULL)
                    {
                        hid_t region_id;    /* region id of the referenced dataset */

                        if((region_id = H5Rget_region(attr_id, H5R_DATASET_REGION, &buf[k])) < 0)
                            goto error;

                        /* create the reference, we need the space_id */
                        if(H5Rcreate(&refbuf[k], fidout, refname, H5R_DATASET_REGION, region_id) < 0)
                            goto error;
                        if(H5Sclose(region_id) < 0)
                            goto error;
                    } /* end if */
                    H5Oclose(refobj_id);
                } /* k */
            } /*nelmts */

            /*-------------------------------------------------------------------------
            * copy
            *-------------------------------------------------------------------------
            */
            if((attr_out = H5Acreate2(loc_out, name, ftype_id, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                goto error;
            if(nelmts)
            {
                if(H5Awrite(attr_out, mtype_id, refbuf) < 0)
                    goto error;
            }

            if(H5Aclose(attr_out) < 0)
                goto error;

            if(refbuf)
                free(refbuf);
            if(buf)
                free(buf);
        } /* H5T_STD_REF_DSETREG */

        /*-------------------------------------------------------------------------
        * close
        *-------------------------------------------------------------------------
        */
        if(H5Tclose(ftype_id) < 0)
            goto error;
        if(H5Tclose(mtype_id) < 0)
            goto error;
        if(H5Sclose(space_id) < 0)
            goto error;
        if(H5Aclose(attr_id) < 0)
            goto error;
    } /* u */

    return 0;

    error:
    H5E_BEGIN_TRY {
            H5Tclose(ftype_id);
            H5Tclose(mtype_id);
            H5Sclose(space_id);
            H5Aclose(attr_id);
            H5Aclose(attr_out);
        } H5E_END_TRY;

    return -1;
}

int copy_attr(hid_t loc_in,
              hid_t loc_out,
              named_dt_t **named_dt_head_p,
              trav_table_t *travt,
              pack_opt_t *options)
{
    hid_t      attr_id=-1;        /* attr ID */
    hid_t      attr_out=-1;       /* attr ID */
    hid_t      space_id=-1;       /* space ID */
    hid_t      ftype_id=-1;       /* file type ID */
    hid_t      wtype_id=-1;       /* read/write type ID */
    size_t     msize;             /* size of type */
    void       *buf=NULL;         /* data buffer */
    hsize_t    nelmts;            /* number of elements in dataset */
    int        rank;              /* rank of dataset */
    htri_t     is_named;          /* Whether the datatype is named */
    hsize_t    dims[H5S_MAX_RANK];/* dimensions of dataset */
    char       name[255];
    H5O_info_t oinfo;             /* object info */
    int        j;
    unsigned   u;

    if(H5Oget_info(loc_in, &oinfo) < 0)
        goto error;

    /*-------------------------------------------------------------------------
    * copy all attributes
    *-------------------------------------------------------------------------
    */

    for ( u = 0; u < (unsigned)oinfo.num_attrs; u++)
    {
        buf=NULL;

        /* open attribute */
        if((attr_id = H5Aopen_by_idx(loc_in, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t)u, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            goto error;

        /* get name */
        if (H5Aget_name( attr_id, (size_t)255, name ) < 0)
            goto error;

        /* get the file datatype  */
        if ((ftype_id = H5Aget_type( attr_id )) < 0 )
            goto error;

        /* Check if the datatype is committed */
        if((is_named = H5Tcommitted(ftype_id)) < 0)
            goto error;
        if(is_named && travt)
        {
            hid_t fidout;

            /* Create out file id */
            if((fidout = H5Iget_file_id(loc_out)) < 0)
                goto error;

            /* Copy named dt */
            if((wtype_id = copy_named_datatype(ftype_id, fidout, named_dt_head_p,
                                               travt, options)) < 0)
            {
                H5Fclose(fidout);
                goto error;
            } /* end if */

            if(H5Fclose(fidout) < 0)
                goto error;
        }
        else
        {
            if (options->use_native==1)
                wtype_id = h5tools_get_native_type(ftype_id);
            else
                wtype_id = H5Tcopy(ftype_id);
        } /* end if */

        /* get the dataspace handle  */
        if ((space_id = H5Aget_space( attr_id )) < 0 )
            goto error;

        /* get dimensions  */
        if ( (rank = H5Sget_simple_extent_dims(space_id, dims, NULL)) < 0 )
            goto error;

        nelmts=1;
        for (j=0; j<rank; j++)
            nelmts*=dims[j];

        if ((msize=H5Tget_size(wtype_id))==0)
            goto error;

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
            if(buf == NULL)
            {
                error_msg("h5repack", "cannot read into memory\n" );
                goto error;
            }
            if(H5Aread(attr_id, wtype_id, buf) < 0)
                goto error;

            /*-------------------------------------------------------------------------
            * copy
            *-------------------------------------------------------------------------
            */

            if((attr_out = H5Acreate2(loc_out, name, wtype_id, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                goto error;
            if(H5Awrite(attr_out, wtype_id, buf) < 0)
                goto error;

            /*close*/
            if(H5Aclose(attr_out) < 0)
                goto error;


            if(buf)
                free(buf);

        } /*H5T_REFERENCE*/


        /*-------------------------------------------------------------------------
        * close
        *-------------------------------------------------------------------------
        */

        if (H5Tclose(ftype_id) < 0) goto error;
        if (H5Tclose(wtype_id) < 0) goto error;
        if (H5Sclose(space_id) < 0) goto error;
        if (H5Aclose(attr_id) < 0) goto error;

    } /* u */


    return 0;

    error:
    H5E_BEGIN_TRY {
            H5Tclose(ftype_id);
            H5Tclose(wtype_id);
            H5Sclose(space_id);
            H5Aclose(attr_id);
            H5Aclose(attr_out);
            if (buf)
                free(buf);
        } H5E_END_TRY;
    return -1;
}




int do_copy_refobjs(hid_t fidin,
                    hid_t fidout,
                    trav_table_t *travt,
                    pack_opt_t *options) /* repack options */
{
    hid_t     grp_in = (-1);          /* read group ID */
    hid_t     grp_out = (-1);         /* write group ID */
    hid_t     dset_in = (-1);         /* read dataset ID */
    hid_t     dset_out = (-1);        /* write dataset ID */
    hid_t     type_in = (-1);         /* named type ID */
    hid_t     dcpl_id = (-1);         /* dataset creation property list ID */
    hid_t     space_id = (-1);        /* space ID */
    hid_t     ftype_id = (-1);        /* file data type ID */
    hid_t     mtype_id = (-1);        /* memory data type ID */
    size_t    msize;                  /* memory size of memory type */
    hsize_t   nelmts;                 /* number of elements in dataset */
    int       rank;                   /* rank of dataset */
    hsize_t   dims[H5S_MAX_RANK];     /* dimensions of dataset */
    unsigned int i, j;
    int       k;
    named_dt_t *named_dt_head=NULL;   /* Pointer to the stack of named datatypes
                                         copied */

    /*-------------------------------------------------------------------------
    * browse
    *-------------------------------------------------------------------------
    */
    for(i = 0; i < travt->nobjs; i++) {
        switch(travt->objs[i].type)
        {
            /*-------------------------------------------------------------------------
            * H5TRAV_TYPE_GROUP
            *-------------------------------------------------------------------------
            */
            case H5TRAV_TYPE_GROUP:
                /*-------------------------------------------------------------------------
                * copy referenced objects in attributes
                *-------------------------------------------------------------------------
                */
                if((grp_out = H5Gopen2(fidout, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;

                if((grp_in = H5Gopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;

                if(copy_refs_attr(grp_in, grp_out, options, travt, fidout) < 0)
                    goto error;

                if(H5Gclose(grp_out) < 0)
                    goto error;
                if(H5Gclose(grp_in) < 0)
                    goto error;

                /*-------------------------------------------------------------------------
                * check for hard links
                *-------------------------------------------------------------------------
                */
                if(travt->objs[i].nlinks)
                    for(j = 0; j < travt->objs[i].nlinks; j++)
                        H5Lcreate_hard(fidout, travt->objs[i].name, H5L_SAME_LOC, travt->objs[i].links[j].new_name, H5P_DEFAULT, H5P_DEFAULT);
                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_DATASET
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_DATASET:
                if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;
                if((space_id = H5Dget_space(dset_in)) < 0)
                    goto error;
                if((ftype_id = H5Dget_type(dset_in)) < 0)
                    goto error;
                if((dcpl_id = H5Dget_create_plist(dset_in)) < 0)
                    goto error;
                if((rank = H5Sget_simple_extent_ndims(space_id)) < 0)
                    goto error;
                if(H5Sget_simple_extent_dims(space_id, dims, NULL) < 0)
                    goto error;
                nelmts = 1;
                for(k = 0; k < rank; k++)
                    nelmts *= dims[k];

                if((mtype_id = h5tools_get_native_type(ftype_id)) < 0)
                    goto error;

                if((msize = H5Tget_size(mtype_id)) == 0)
                    goto error;


                if(h5tools_canreadf(NULL, dcpl_id) == 1) {
                    dset_out = FAIL;

                    if(H5Tequal(mtype_id, H5T_STD_REF_OBJ)) {
                        hid_t            refobj_id;
                        hobj_ref_t       *refbuf = NULL; /* buffer for object references */
                        hobj_ref_t       *buf = NULL;
                        const char*      refname;
                        unsigned         u;


                        if(nelmts) {
                            buf = (hobj_ref_t *)malloc((unsigned)(nelmts * msize));
                            if(buf==NULL) {
                                printf("cannot read into memory\n" );
                                goto error;
                            } /* end if */
                            if(H5Dread(dset_in, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0)
                                goto error;

                            refbuf = (hobj_ref_t*) calloc((unsigned)nelmts, msize);
                            if(refbuf == NULL){
                                printf("cannot allocate memory\n" );
                                goto error;
                            } /* end if */
                            for(u = 0; u < nelmts; u++) {
                                H5E_BEGIN_TRY {
                                        if((refobj_id = H5Rdereference1(dset_in, H5R_OBJECT, &buf[u])) < 0)
                                            continue;
                                    } H5E_END_TRY;

                                /* get the name. a valid name could only occur
                                 * in the second traversal of the file
                                 */
                                if((refname = MapIdToName(refobj_id, travt)) != NULL) {
                                    /* create the reference, -1 parameter for objects */
                                    if(H5Rcreate(&refbuf[u], fidout, refname, H5R_OBJECT, -1) < 0)
                                        goto error;
                                } /*refname*/
                                H5Oclose(refobj_id);
                            } /* u */
                        } /*nelmts*/

                        if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, mtype_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                            goto error;
                        if(nelmts)
                            if(H5Dwrite(dset_out, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, refbuf) < 0)
                                goto error;

                        if(buf)
                            free(buf);
                        if(refbuf)
                            free(refbuf);

                        if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                            goto error;
                    } /*H5T_STD_REF_OBJ*/

                    else if(H5Tequal(mtype_id, H5T_STD_REF_DSETREG))
                    {
                        hid_t            refobj_id;
                        hdset_reg_ref_t  *refbuf = NULL; /* input buffer for region references */
                        hdset_reg_ref_t  *buf = NULL;    /* output buffer */
                        const char*      refname;
                        unsigned         u;

                        if(nelmts) {
                            buf = (hdset_reg_ref_t *)malloc((unsigned)(nelmts * msize));
                            if(buf == NULL) {
                                printf("cannot read into memory\n");
                                goto error;
                            } /* end if */
                            if(H5Dread(dset_in, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0)
                                goto error;

                            refbuf = (hdset_reg_ref_t *)calloc(sizeof(hdset_reg_ref_t), (size_t)nelmts); /*init to zero */
                            if(refbuf == NULL) {
                                printf("cannot allocate memory\n");
                                goto error;
                            } /* end if */

                            for(u = 0; u < nelmts; u++) {
                                H5E_BEGIN_TRY {
                                        if((refobj_id = H5Rdereference1(dset_in, H5R_DATASET_REGION, &buf[u])) < 0)
                                            continue;
                                    } H5E_END_TRY;

                                /* get the name. a valid name could only occur
                                 * in the second traversal of the file
                                 */
                                if((refname = MapIdToName(refobj_id, travt)) != NULL) {
                                    hid_t region_id;    /* region id of the referenced dataset */

                                    if((region_id = H5Rget_region(dset_in, H5R_DATASET_REGION, &buf[u])) < 0)
                                        goto error;

                                    /* create the reference, we need the space_id */
                                    if(H5Rcreate(&refbuf[u], fidout, refname, H5R_DATASET_REGION, region_id) < 0)
                                        goto error;
                                    if(H5Sclose(region_id) < 0)
                                        goto error;

                                } /*refname*/
                                H5Oclose(refobj_id);
                            } /* u */
                        } /*nelmts*/

                        if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, mtype_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                            goto error;
                        if(nelmts)
                            if(H5Dwrite(dset_out, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, refbuf) < 0)
                                goto error;

                        if(buf)
                            free(buf);
                        if(refbuf)
                            free(refbuf);

                        if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                            goto error;
                    } /* H5T_STD_REF_DSETREG */

                    else {
                        if((dset_out = H5Dopen2(fidout, travt->objs[i].name, H5P_DEFAULT)) < 0)
                            goto error;
                    } /* end else */

                    if(copy_refs_attr(dset_in, dset_out, options, travt, fidout) < 0)
                        goto error;

                    if(travt->objs[i].nlinks)
                        for(j = 0; j < travt->objs[i].nlinks; j++)
                            H5Lcreate_hard(fidout, travt->objs[i].name, H5L_SAME_LOC, travt->objs[i].links[j].new_name, H5P_DEFAULT, H5P_DEFAULT);

                    if(H5Dclose(dset_out) < 0)
                        goto error;
                } /*can_read*/

                if(H5Tclose(ftype_id) < 0)
                    goto error;
                if(H5Tclose(mtype_id) < 0)
                    goto error;
                if(H5Pclose(dcpl_id) < 0)
                    goto error;
                if(H5Sclose(space_id) < 0)
                    goto error;
                if(H5Dclose(dset_in) < 0)
                    goto error;
                break;

            case H5TRAV_TYPE_NAMED_DATATYPE:
                if((type_in = H5Topen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;
                if(H5Tclose(type_in) < 0)
                    goto error;
                break;

            case H5TRAV_TYPE_LINK:
                /*nothing to do */
                break;

            case H5TRAV_TYPE_UNKNOWN:
            case H5TRAV_TYPE_UDLINK:
                goto error;

            default:
                break;
        } /* end switch */
    } /* end for */

    named_datatype_free(&named_dt_head, 0);

    return 0;

    error:
    H5E_BEGIN_TRY {
            H5Gclose(grp_in);
            H5Gclose(grp_out);
            H5Pclose(dcpl_id);
            H5Sclose(space_id);
            H5Dclose(dset_in);
            H5Dclose(dset_out);
            H5Tclose(ftype_id);
            H5Tclose(mtype_id);
            H5Tclose(type_in);
            named_datatype_free(&named_dt_head, 0);
        } H5E_END_TRY;

    return -1;
}

static int   do_copy_objects(H5File infile,hid_t fidout,trav_table_t *travt,pack_opt_t *options);

int copy_objects(H5File file,
                 const char* fnameout,
                 pack_opt_t *options)
{
    hid_t         fidin;
    hid_t         fidout = -1;
    trav_table_t  *travt = NULL;
    hsize_t       ub_size = 0;        /* size of user block */
    hid_t         fcpl = H5P_DEFAULT; /* file creation property list ID */
    hid_t         fapl = H5P_DEFAULT; /* file access property list ID */

    fidin = file.getId();

    /* get user block size */
    {
        hid_t fcpl_in; /* file creation property list ID for input file */

        if((fcpl_in = H5Fget_create_plist(fidin)) < 0)
        {
            error_msg("failed to retrieve file creation property list\n");
            goto out;
        }

        if(H5Pget_userblock(fcpl_in, &ub_size) < 0)
        {
            error_msg("failed to retrieve userblock size\n");
            goto out;
        }

        if(H5Pclose(fcpl_in) < 0)
        {
            error_msg("failed to close property list\n");
            goto out;
        }
    }

    if((fidout = H5Fcreate(fnameout,H5F_ACC_TRUNC, fcpl, fapl)) < 0)
    {
        error_msg("<%s>: Could not create file\n", fnameout );
        goto out;
    }

    /* init table */
    trav_table_init(&travt);

    /* get the list of objects in the file */
    if(h5trav_gettable(fidin, travt) < 0)
        goto out;

    do_copy_objects(file, fidout, travt, options) ;
    do_copy_refobjs(fidin, fidout, travt, options);

    if(fapl > 0)
        H5Pclose(fapl);

    if(fcpl > 0)
        H5Pclose(fcpl);

    H5Fclose(fidin);
    H5Fclose(fidout);

    /* free table */
    trav_table_free(travt);
    travt = NULL;

    return 0;


    out:
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

int do_copy_objects(H5File infile,
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
    hid_t    fidin = infile.getId();
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
                    goto error;

                /* get input group creation property list */
                if ((gcpl_in = H5Gget_create_plist(grp_in)) < 0)
                    goto error;

                /* query and set the group creation properties */
                if (H5Pget_link_creation_order(gcpl_in, &crt_order_flags) < 0)
                    goto error;

                /* set up group creation property list */
                if ((gcpl_out = H5Pcreate(H5P_GROUP_CREATE)) < 0)
                    goto error;

                if (H5Pset_link_creation_order(gcpl_out, crt_order_flags) < 0)
                    goto error;


                /*-------------------------------------------------------------------------
                * the root is a special case, we get an ID for the root group
                * and copy its attributes using that ID
                *-------------------------------------------------------------------------
                */
                if(strcmp(travt->objs[i].name, "/") == 0)
                {
                    if ((grp_out = H5Gopen2(fidout, "/", H5P_DEFAULT)) < 0)
                        goto error;
                }

                else
                {

                    if (options->grp_compact>0 || options->grp_indexed>0)
                    {
                        if(H5Pset_link_phase_change(gcpl_out, (unsigned)options->grp_compact, (unsigned)options->grp_indexed) < 0)
                            goto error;
                    }

                    if((grp_out = H5Gcreate2(fidout, travt->objs[i].name, H5P_DEFAULT, gcpl_out, H5P_DEFAULT)) < 0)
                        goto error;

                }

                if(copy_attr(grp_in, grp_out, &named_dt_head, travt, options) < 0)
                    goto error;


                if(H5Pclose(gcpl_out) < 0)
                    goto error;
                if(H5Pclose(gcpl_in) < 0)
                    goto error;
                if(H5Gclose(grp_out) < 0)
                    goto error;
                if(H5Gclose(grp_in) < 0)
                    goto error;

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
                    goto error;
                if((ftype_id = H5Dget_type(dset_in)) < 0)
                    goto error;
                if(H5T_REFERENCE == H5Tget_class(ftype_id))
                    is_ref = 1;

                /* Check if the datatype is committed */
                if((is_named = H5Tcommitted(ftype_id)) < 0)
                    goto error;
                if(is_named)
                    if((wtype_id = copy_named_datatype(ftype_id, fidout, &named_dt_head, travt, options)) < 0)
                        goto error;

                if(H5Tclose(ftype_id) < 0)
                    goto error;
                if(H5Dclose(dset_in) < 0)
                    goto error;

                if ( options->op_tbl->nelems  ||
                     options->all_filter == 1 ||
                     options->all_layout == 1 ||
                     is_ref ||
                     is_named)
                {

                    int      j;

                    if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        goto error;
                    if((f_space_id = H5Dget_space(dset_in)) < 0)
                        goto error;
                    if((ftype_id = H5Dget_type(dset_in)) < 0)
                        goto error;
                    if((dcpl_id = H5Dget_create_plist(dset_in)) < 0)
                        goto error;
                    if((dcpl_out = H5Pcopy(dcpl_id)) < 0)
                        goto error;
                    if((rank = H5Sget_simple_extent_ndims(f_space_id)) < 0)
                        goto error;
                    memset(dims, 0, sizeof dims);
                    if(H5Sget_simple_extent_dims(f_space_id, dims, NULL) < 0)
                        goto error;
                    nelmts = 1;
                    for ( j = 0; j < rank; j++)
                    {
                        nelmts *= dims[j];
                    }

                    /* wtype_id will have already been set if using a named dtype */
                    if(!is_named) {
                        if(options->use_native == 1)
                            wtype_id = h5tools_get_native_type(ftype_id);
                        else
                            wtype_id = H5Tcopy(ftype_id);
                    } /* end if */

                    if((msize = H5Tget_size(wtype_id)) == 0)
                        goto error;

                    if (h5tools_canreadf((travt->objs[i].name),dcpl_id)==1)
                    {
                        apply_s=1;
                        apply_f=1;

                        if (H5T_REFERENCE!=H5Tget_class(wtype_id))
                        {
                            /* get the storage size of the input dataset */
                            dsize_in=H5Dget_storage_size(dset_in);

                            if (options->layout_g != H5D_COMPACT)
                            {
                                if ( nelmts*msize < options->min_comp )
                                    apply_s=0;
                            }

                            /* apply the filter */
                            if (apply_s)
                            {
                                if (apply_filters(travt->objs[i].name,
                                                  rank,
                                                  dims,
                                                  msize,
                                                  dcpl_out,
                                                  options,
                                                  &has_filter) < 0)
                                    goto error;
                            }

                            H5E_BEGIN_TRY {
                                    dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_out, H5P_DEFAULT);
                                } H5E_END_TRY;

                            if(dset_out == FAIL)
                            {
                                if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                                    goto error;
                                apply_f = 0;
                            }

                            if (nelmts)
                            {
                                size_t need = (size_t)(nelmts*msize);  /* bytes needed */
                                if ( need < H5TOOLS_MALLOCSIZE )
                                    buf = malloc(need);

                                if (buf != NULL )
                                {
                                    if (H5Dread(dset_in,wtype_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf) < 0)
                                        goto error;
                                    if (H5Dwrite(dset_out,wtype_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf) < 0)
                                        goto error;
                                }
                            }

                            copy_attr(dset_in, dset_out, &named_dt_head, travt, options);
                            H5Dclose(dset_out);

                        }/*!H5T_REFERENCE*/
                    }/*h5tools_canreadf*/


                    /*-------------------------------------------------------------------------
                    * close
                    *-------------------------------------------------------------------------
                    */
                    if (H5Tclose(ftype_id) < 0)
                        goto error;
                    if (H5Tclose(wtype_id) < 0)
                        goto error;
                    if (H5Pclose(dcpl_id) < 0)
                        goto error;
                    if (H5Pclose(dcpl_out) < 0)
                        goto error;
                    if (H5Sclose(f_space_id) < 0)
                        goto error;
                    if (H5Dclose(dset_in) < 0)
                        goto error;

                }

                else
                {
                    hid_t        pid;

                    /* create property to pass copy options */
                    if ( (pid = H5Pcreate(H5P_OBJECT_COPY)) < 0)
                        goto error;

                    /* set options for object copy */
                    if(H5Pset_copy_object(pid, H5O_COPY_WITHOUT_ATTR_FLAG) < 0)
                        goto error;

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
                        goto error;

                    /* close property */
                    if(H5Pclose(pid) < 0)
                        goto error;


                    /*-------------------------------------------------------------------------
                    * copy attrs manually
                    *-------------------------------------------------------------------------
                    */
                    if((dset_in = H5Dopen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        goto error;
                    if((dset_out = H5Dopen2(fidout, travt->objs[i].name, H5P_DEFAULT)) < 0)
                        goto error;
                    if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                        goto error;
                    if(H5Dclose(dset_in) < 0)
                        goto error;
                    if(H5Dclose(dset_out) < 0)
                        goto error;



                } /* end do we have request for filter/chunking */


                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_NAMED_DATATYPE
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_NAMED_DATATYPE:

                if((type_in = H5Topen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;

                /* Copy the datatype anonymously */
                if((type_out = copy_named_datatype(type_in, fidout, &named_dt_head,
                                                   travt, options)) < 0)
                    goto error;

                /* Link in to group structure */
                if(H5Lcreate_hard(type_out, ".", fidout, travt->objs[i].name,
                                  H5P_DEFAULT, H5P_DEFAULT) < 0)
                    goto error;

                /*-------------------------------------------------------------------------
                * copy attrs
                *-------------------------------------------------------------------------
                */
                if(copy_attr(type_in, type_out, &named_dt_head, travt, options) < 0)
                    goto error;

                if(H5Tclose(type_in) < 0)
                    goto error;
                if(H5Tclose(type_out) < 0)
                    goto error;



                break;

            case H5TRAV_TYPE_LINK:
            case H5TRAV_TYPE_UDLINK:
            {

                if(H5Lcopy(fidin, travt->objs[i].name,fidout, travt->objs[i].name, H5P_DEFAULT, H5P_DEFAULT) < 0)
                    goto error;

            }
                break;

            default:
                goto error;
        } /* switch */

        /* free */
        if (buf!=NULL)
        {
            free(buf);
            buf=NULL;
        }

    } /* i */

    /* Finalize (link) the stack of named datatypes (if any) */
    named_datatype_free(&named_dt_head, 0);

    return 0;

    error:
    H5E_BEGIN_TRY {
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
        } H5E_END_TRY;
    /* free */
    if (buf!=NULL)
        free(buf);
    if (sm_buf!=NULL)
        free(sm_buf);
    return -1;
}

int options_table_init( pack_opttbl_t **tbl )
{
    unsigned int i;
    pack_opttbl_t *table;

    if(NULL == (table = (pack_opttbl_t *)malloc(sizeof(pack_opttbl_t))))
    {
        error_msg("not enough memory for options table\n");
        return -1;
    }

    table->size   = 30;
    table->nelems = 0;
    if(NULL == (table->objs = (pack_info_t*)malloc(table->size * sizeof(pack_info_t))))
    {
        error_msg("not enough memory for options table\n");
        free(table);
        return -1;
    }

    for(i = 0; i < table->size; i++)
        init_packobject(&table->objs[i]);

    *tbl = table;
    return 0;
}

void h5repack_init (pack_opt_t *options, int verbose)
{
    int k, n;
    memset(options,0,sizeof(pack_opt_t));
    options->min_comp = 1024;

    for ( n = 0; n < H5_REPACK_MAX_NFILTERS; n++)
    {
        options->filter_g[n].filtn  = -1;
        options->filter_g[n].cd_nelmts  = 0;
        for ( k = 0; k < CD_VALUES; k++)
            options->filter_g[n].cd_values[k] = 0;
    }

    options_table_init(&(options->op_tbl));
}

int h5repack::repack(H5File inputFile, string outputFile, string gzipCompression)
{
    pack_opt_t    options;
    h5repack_init(&options,0);

    opt_ind = opt_ind + 2;

    string compression = "GZIP=";
    compression = compression.append(gzipCompression);

    h5repack_addfilter( compression.c_str(), &options);
    copy_objects(inputFile,outputFile.c_str(), &options);

    h5repack_end(&options);
    return 0;
}