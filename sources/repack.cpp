#include "../headers/repack.hpp"
#include <unistd.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <fstream>


using namespace std;

int         opt_ind = 1;
const char *opt_arg;

const hsize_t H5TOOLS_BUFSIZE = 33554432;  /* 32 MB */

void error_msg(string errorMessage){

}

void error_msg(string errorMessage, string cuco){

}

void error_msg(string errorMessage, string cuco, string coca){

}

void error_msg(string errorMessage, string cuco, string coca, string coquito){

}


void print_warning(string errorMessage, string cuco){

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

static hid_t
h5tools_get_fapl(hid_t fapl, const char *driver, unsigned *drivernum)
{
    hid_t new_fapl; /* Copy of file access property list passed in, or new property list */

    /* Make a copy of the FAPL, for the file open call to use, eventually */
    if (fapl == H5P_DEFAULT) {
        if ((new_fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
            goto error;
    } /* end if */
    else {
        if ((new_fapl = H5Pcopy(fapl)) < 0)
            goto error;
    } /* end else */

    /* Determine which driver the user wants to open the file with. Try
     * that driver. If it can't open it, then fail. */
    if (!strcmp(driver, drivernames[SEC2_IDX])) {
        /* SEC2 driver */
        if (H5Pset_fapl_sec2(new_fapl) < 0)
            goto error;

        if (drivernum)
            *drivernum = SEC2_IDX;
    }
    else if (!strcmp(driver, drivernames[FAMILY_IDX])) {
        /* FAMILY Driver */

        /* Set member size to be 0 to indicate the current first member size
         * is the member size.
         */
        if (H5Pset_fapl_family(new_fapl, (hsize_t) 0, H5P_DEFAULT) < 0)
            goto error;

        if (drivernum)
            *drivernum = FAMILY_IDX;
    }
    else if (!strcmp(driver, drivernames[SPLIT_IDX])) {
        /* SPLIT Driver */
        if (H5Pset_fapl_split(new_fapl, "-m.h5", H5P_DEFAULT, "-r.h5", H5P_DEFAULT) < 0)
            goto error;

        if (drivernum)
            *drivernum = SPLIT_IDX;
    }
    else if (!strcmp(driver, drivernames[MULTI_IDX])) {
        /* MULTI Driver */
        if (H5Pset_fapl_multi(new_fapl, NULL, NULL, NULL, NULL, true) < 0)
            goto error;

        if(drivernum)
            *drivernum = MULTI_IDX;
#ifdef H5_HAVE_STREAM
        }
            else if(!strcmp(driver, drivernames[STREAM_IDX])) {
                /* STREAM Driver */
                if(H5Pset_fapl_stream(new_fapl, NULL) < 0)
                goto error;

                if(drivernum)
                *drivernum = STREAM_IDX;
#endif /* H5_HAVE_STREAM */
#ifdef H5_HAVE_PARALLEL
        }
            else if(!strcmp(driver, drivernames[MPIO_IDX])) {
                /* MPI-I/O Driver */
                /* check if MPI has been initialized. */
                if(!h5tools_mpi_init_g)
                MPI_Initialized(&h5tools_mpi_init_g);
                if(h5tools_mpi_init_g) {
                    if(H5Pset_fapl_mpio(new_fapl, MPI_COMM_WORLD, MPI_INFO_NULL) < 0)
                goto error;

            if(drivernum)
                *drivernum = MPIO_IDX;
        } /* end if */
    }
    else if (!strcmp(driver, drivernames[MPIPOSIX_IDX])) {
        /* MPI-I/O Driver */
        /* check if MPI has been initialized. */
        if(!h5tools_mpi_init_g)
            MPI_Initialized(&h5tools_mpi_init_g);
        if(h5tools_mpi_init_g) {
            if(H5Pset_fapl_mpiposix(new_fapl, MPI_COMM_WORLD, TRUE) < 0)
                goto error;

            if(drivernum)
                *drivernum = MPIPOSIX_IDX;
        } /* end if */
#endif /* H5_HAVE_PARALLEL */
    }
    else {
        goto error;
    }

    return(new_fapl);

    error:
    if(new_fapl != H5P_DEFAULT)
        H5Pclose(new_fapl);
    return -1;
}


hid_t
h5tools_fopen(const char *fname, unsigned flags, hid_t fapl, const char *driver,
              char *drivername, size_t drivername_size)
{
    unsigned    drivernum;
    hid_t       fid = FAIL;
    hid_t       my_fapl = H5P_DEFAULT;

    if (driver && *driver) {
        /* Get the correct FAPL for the given driver */
        if ((my_fapl = h5tools_get_fapl(fapl, driver, &drivernum)) < 0)
            goto done;

        H5E_BEGIN_TRY {
                fid = H5Fopen(fname, flags, my_fapl);
            } H5E_END_TRY;

        if (fid == FAIL)
            goto done;

    }
    else {
        /* Try to open the file using each of the drivers */
        for (drivernum = 0; drivernum < NUM_DRIVERS; drivernum++) {
            /* Get the correct FAPL for the given driver */
            if((my_fapl = h5tools_get_fapl(fapl, drivernames[drivernum], NULL)) < 0)
                goto done;

            H5E_BEGIN_TRY {
                    fid = H5Fopen(fname, flags, my_fapl);
                } H5E_END_TRY;

            if (fid != FAIL)
                break;
            else {
                /* Close the FAPL */
                H5Pclose(my_fapl);
                my_fapl = H5P_DEFAULT;
            } /* end else */
        }
    }

    /* Save the driver name */
    if (drivername && drivername_size) {
        if (fid != FAIL) {
            strncpy(drivername, drivernames[drivernum], drivername_size);
            drivername[drivername_size - 1] = '\0';
        }
        else {
            /*no file opened*/
            drivername[0] = '\0';
        }
    }

    done:
    if(my_fapl != H5P_DEFAULT)
        H5Pclose(my_fapl);

    return fid;
}


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


/*-------------------------------------------------------------------------
 * Function: trav_addr_visited
 *
 * Purpose: Check if an address has already been visited
 *
 * Return: true/FALSE
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
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


/*-------------------------------------------------------------------------
 * Function: traverse_cb
 *
 * Purpose: Iterator callback for traversing objects in file
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
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


/*-------------------------------------------------------------------------
 * Function: traverse
 *
 * Purpose: Iterate over all the objects/links in a file.  Conforms to the
 *      "visitor" pattern.
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
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



/*-------------------------------------------------------------------------
 * Function: trav_info_add
 *
 * Purpose: Add a link path & type to info struct
 *
 * Return: void
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
void
trav_info_add(trav_info_t *info, const char *path, h5trav_type_t obj_type)
{
    size_t idx;         /* Index of address to use  */

    /* Allocate space if necessary */
    if(info->nused == info->nalloc) {
        info->nalloc = MAX(1, info->nalloc * 2);;
        info->paths = (trav_path_t *)realloc(info->paths, info->nalloc * sizeof(trav_path_t));
    } /* end if */

    /* Append it */
    idx = info->nused++;
    info->paths[idx].path = strdup(path);
    info->paths[idx].type = obj_type;
} /* end trav_info_add() */


/*-------------------------------------------------------------------------
 * Function: trav_info_visit_obj
 *
 * Purpose: Callback for visiting object, with 'info' structure
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
int
trav_info_visit_obj(const char *path, const H5O_info_t *oinfo,
                    const char *already_visited, void *udata)
{
/* Add the object to the 'info' struct */
/* (object types map directly to "traversal" types) */
trav_info_add((trav_info_t *)udata, path, (h5trav_type_t)oinfo->type);

return(0);
} /* end trav_info_visit_obj() */


/*-------------------------------------------------------------------------
 * Function: trav_info_visit_lnk
 *
 * Purpose: Callback for visiting link, with 'info' structure
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */

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





int
trav_info_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata)
{
    /* Add the link to the 'info' struct */
    trav_info_add((trav_info_t *)udata, path, ((linfo->type == H5L_TYPE_SOFT) ? H5TRAV_TYPE_LINK : H5TRAV_TYPE_UDLINK));

    return(0);
} /* end trav_info_visit_lnk() */


/*-------------------------------------------------------------------------
 * Function: h5trav_getinfo
 *
 * Purpose: get an array of "trav_info_t" , containing the name and type of
 *  objects in the file
 *
 * Return: number of object names in file
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: November 6, 2002
 *
 *-------------------------------------------------------------------------
 */
int
h5trav_getinfo(hid_t file_id, trav_info_t *info)
{
    trav_visitor_t info_visitor;        /* Visitor structure for trav_info_t's */

    /* Init visitor structure */
    info_visitor.visit_obj = trav_info_visit_obj;
    info_visitor.visit_lnk = trav_info_visit_lnk;
    info_visitor.udata = info;

    /* Traverse all objects in the file, visiting each object & link */
    if(traverse(file_id, "/", true, true, &info_visitor) < 0)
        return -1;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function: h5trav_getindex
 *
 * Purpose: get index of OBJ in list
 *
 * Return: index, -1 if not found
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: May 9, 2003
 *
 *-------------------------------------------------------------------------
 */

ssize_t
h5trav_getindex(const trav_info_t *info, const char *obj)
{
    size_t u;           /* Local index variable */

    /* Loop over all paths in 'info' struct, looking for object */
    for(u = 0; u < info->nused; u++) {
        /* Check for object name having full path (with leading '/') */
        if(strcmp(obj, info->paths[u].path) == 0)
            return((ssize_t)u);

        /* Check for object name without leading '/' */
        if(strcmp(obj, (info->paths[u].path + 1)) == 0)
            return((ssize_t)u);
    } /* end for */

    return((ssize_t)-1);
} /* end h5trav_getindex() */


/*-------------------------------------------------------------------------
 * Function: trav_info_init
 *
 * Purpose: Initialize the info
 *
 * Return: void
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 6, 2007
 *
 *-------------------------------------------------------------------------
 */

void
trav_info_init(const char *filename, hid_t fileid, trav_info_t **_info)
{
    trav_info_t *info = (trav_info_t *)malloc(sizeof(trav_info_t));

    /* Init info structure */
    info->nused = info->nalloc = 0;
    info->paths = NULL;
    info->fname = filename;
    info->fid = fileid;

    /* Initialize list of visited symbolic links */
    info->symlink_visited.nused = 0;
    info->symlink_visited.nalloc = 0;
    info->symlink_visited.objs = NULL;
    info->symlink_visited.dangle_link = false;
    *_info = info;
} /* end trav_info_init() */


/*-------------------------------------------------------------------------
 * Function: trav_info_free
 *
 * Purpose: free info memory
 *
 *-------------------------------------------------------------------------
 */

void
trav_info_free(trav_info_t *info)
{
    size_t u;           /* Local index variable */

    if(info) {
        /* Free visited symbolic links path and file (if alloc) */
        for(u=0; u < info->symlink_visited.nused; u++)
        {
            if (info->symlink_visited.objs[u].file)
                free(info->symlink_visited.objs[u].file);

            free(info->symlink_visited.objs[u].path);
        }
        free(info->symlink_visited.objs);

        /* Free path names */
        for(u = 0; u < info->nused; u++)
            free(info->paths[u].path);
        free(info->paths);
        free(info);
    } /* end if */
} /* end trav_info_free() */

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

/*-------------------------------------------------------------------------
 * "h5trav table" public functions. used in h5repack
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function: trav_table_visit_obj
 *
 * Purpose: Callback for visiting object, with 'table' sructure
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
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


/*-------------------------------------------------------------------------
 * Function: trav_table_visit_lnk
 *
 * Purpose: Callback for visiting link, with 'table' sructure
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 1, 2007
 *
 *-------------------------------------------------------------------------
 */
static int
trav_table_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata)
{
/* Add the link to the 'table' struct */
trav_table_add((trav_table_t *)udata, path, NULL);

return(0);
} /* end trav_table_visit_lnk() */


/*-------------------------------------------------------------------------
 * Function: h5trav_gettable
 *
 * Purpose: get the trav_table_t struct
 *
 * Return: 0, -1 on error
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: December 17, 2003
 *
 *-------------------------------------------------------------------------
 */

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

/*-------------------------------------------------------------------------
 * Function: h5trav_getindext
 *
 * Purpose: get index of NAME in list
 *
 * Return: index, -1 if not found
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: December 18, 2003
 *
 *-------------------------------------------------------------------------
 */

int
h5trav_getindext(const char *name, const trav_table_t *table)
{
    unsigned int i;

    for(i = 0; i < table->nobjs; i++) {
        /* Check for object name having full path (with leading '/') */
        if(strcmp(name, table->objs[i].name) == 0)
            return(i);

        /* Check for object name without leading '/' */
        if(strcmp(name, table->objs[i].name + 1) == 0)
            return(i);

        /* search also in the list of links */
        if(table->objs[i].nlinks) {
            unsigned int j;

            for ( j=0; j<table->objs[i].nlinks; j++) {
                /* Check for object name having full path (with leading '/') */
                if(strcmp(name, table->objs[i].links[j].new_name) == 0)
                    return(i);

                /* Check for object name without leading '/' */
                if(strcmp(name, table->objs[i].links[j].new_name + 1) == 0)
                    return(i);
            } /* end for */
        } /* end if */
    } /* end for */

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: trav_table_add
 *
 * Purpose: Add OBJNO, NAME and TYPE of object to table
 *
 * Return: void
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: November 4, 2002
 *
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function: trav_table_addlink
 *
 * Purpose: Add a hardlink name to the object
 *
 * Return: void
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: December 17, 2003
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: trav_table_addflags
 *
 * Purpose: Add FLAGS, NAME and TYPE of object to table
 *
 * Return: void
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: November 4, 2002
 *
 *-------------------------------------------------------------------------
 */



void trav_table_addflags(unsigned *flags,
                         char *name,
                         h5trav_type_t type,
                         trav_table_t *table)
{
    unsigned int newObj;

    if(table->nobjs == table->size) {
        table->size = MAX(1, table->size * 2);
        table->objs = (trav_obj_t *)realloc(table->objs, table->size * sizeof(trav_obj_t));
    } /* end if */

    newObj = table->nobjs++;
    table->objs[newObj].objno = 0;
    table->objs[newObj].flags[0] = flags[0];
    table->objs[newObj].flags[1] = flags[1];
    table->objs[newObj].name = (char *)strdup(name);
    table->objs[newObj].type = type;
    table->objs[newObj].nlinks = 0;
    table->objs[newObj].sizelinks = 0;
    table->objs[newObj].links = NULL;
}


/*-------------------------------------------------------------------------
 * Function: trav_table_init
 *
 * Purpose: Initialize the table
 *
 * Return: void
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: November 4, 2002
 *
 *-------------------------------------------------------------------------
 */

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

/*-------------------------------------------------------------------------
 * Function: trav_table_free
 *
 * Purpose: free table memory
 *
 * Return: void
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: November 4, 2002
 *
 *-------------------------------------------------------------------------
 */

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


/*-------------------------------------------------------------------------
 * Function: trav_print_visit_obj
 *
 * Purpose: Callback for visiting object, when printing info
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 6, 2007
 *
 *-------------------------------------------------------------------------
 */
static int
trav_print_visit_obj(const char *path, const H5O_info_t *oinfo,
                     const char *already_visited, void *udata)
{
/* Print the name of the object */
/* (no new-line, so that objects that we've encountered before can print
 *  the name of the original object)
 */
switch(oinfo->type) {
case H5O_TYPE_GROUP:
printf(" %-10s %s", "group", path);
break;

case H5O_TYPE_DATASET:
printf(" %-10s %s", "dataset", path);
break;

case H5O_TYPE_NAMED_DATATYPE:
printf(" %-10s %s", "datatype", path);
break;

default:
printf(" %-10s %s", "unknown object type", path);
break;
} /* end switch */

/* Check if we've already seen this object */
if(NULL == already_visited)
/* Finish printing line about object */
printf("\n");
else
/* Print the link's original name */
printf(" -> %s\n", already_visited);

return(0);
} /* end trav_print_visit_obj() */


/*-------------------------------------------------------------------------
 * Function: trav_print_visit_lnk
 *
 * Purpose: Callback for visiting link, when printing info
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 6, 2007
 *
 *-------------------------------------------------------------------------
 */

static void aux_tblinsert_filter(pack_opttbl_t *table,
                                 unsigned int I,
                                 filter_info_t filt)
{
    if (table->objs[ I ].nfilters<H5_REPACK_MAX_NFILTERS)
    {
        table->objs[ I ].filter[ table->objs[ I ].nfilters++ ] = filt;
    }
    else
    {
        error_msg("cannot insert the filter in this object.\
        Maximum capacity exceeded\n");
    }
}



static void aux_tblinsert_layout(pack_opttbl_t *table,
                                 unsigned int I,
                                 pack_info_t *pack)
{
    int k;

    table->objs[I].layout = pack->layout;
    if (H5D_CHUNKED==pack->layout)
    {
        /* -2 means the NONE option, remove chunking
            and set the layout to contiguous */
        if (pack->chunk.rank==-2)
        {
            table->objs[I].layout = H5D_CONTIGUOUS;
            table->objs[I].chunk.rank = -2;
        }
            /* otherwise set the chunking type */
        else
        {
            table->objs[I].chunk.rank = pack->chunk.rank;
            for (k = 0; k < pack->chunk.rank; k++)
                table->objs[I].chunk.chunk_lengths[k] = pack->chunk.chunk_lengths[k];
        }
    }
}




static int
trav_print_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata)
{
    trav_print_udata_t *print_udata = (trav_print_udata_t *)udata;

    /* Print appropriate information for the type of link */
    switch(linfo->type) {
        case H5L_TYPE_SOFT:
            if(linfo->u.val_size > 0) {
                char *targbuf = (char*)malloc(linfo->u.val_size + 1);
                assert(targbuf);

                H5Lget_val(print_udata->fid, path, targbuf, linfo->u.val_size + 1, H5P_DEFAULT);
                printf(" %-10s %s -> %s\n", "link", path, targbuf);
                free(targbuf);
            } /* end if */
            else
                printf(" %-10s %s ->\n", "link", path);
            break;

        case H5L_TYPE_EXTERNAL:
            if(linfo->u.val_size > 0) {
                char *targbuf;
                const char *filename;
                const char *objname;

                targbuf = (char*)malloc(linfo->u.val_size + 1);
                assert(targbuf);

                H5Lget_val(print_udata->fid, path, targbuf, linfo->u.val_size + 1, H5P_DEFAULT);
                H5Lunpack_elink_val(targbuf, linfo->u.val_size, NULL, &filename, &objname);
                printf(" %-10s %s -> %s %s\n", "ext link", path, filename, objname);
                free(targbuf);
            } /* end if */
            else
                printf(" %-10s %s ->\n", "ext link", path);
            break;

        default:
            printf(" %-10s %s -> ???\n", "unknown type of UD link", path);
            break;
    } /* end switch() */

    return(0);
} /* end trav_print_visit_lnk() */

static const char* get_sfilter(H5Z_filter_t filtn)
{
    if (filtn==H5Z_FILTER_NONE)
        return "NONE";
    else if (filtn==H5Z_FILTER_DEFLATE)
        return "GZIP";
    else if (filtn==H5Z_FILTER_SZIP)
        return "SZIP";
    else if (filtn==H5Z_FILTER_SHUFFLE)
        return "SHUFFLE";
    else if (filtn==H5Z_FILTER_FLETCHER32)
        return "FLETCHER32";
    else if (filtn==H5Z_FILTER_NBIT)
        return "NBIT";
    else if (filtn==H5Z_FILTER_SCALEOFFSET)
        return "SOFF";
    else {
        error_msg("input error in filter type\n");
        exit(EXIT_FAILURE);
    }
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
    for (i=0; i<nfilters; i++)
    {
        if ((filtn = H5Pget_filter(dcpl_id,(unsigned)i,0,0,0,0,0,0))<0)
            return -1;

        switch (filtn)
        {
/*-------------------------------------------------------------------------
 * user defined filter
 *-------------------------------------------------------------------------
 */
            default:
                if (name)
                    print_warning(name,"user defined");
                return 0;

/*-------------------------------------------------------------------------
 * H5Z_FILTER_DEFLATE      1 , deflation like gzip
 *-------------------------------------------------------------------------
 */
            case H5Z_FILTER_DEFLATE:
                if (!have_deflate)
                {
                    if (name)
                        print_warning(name,"deflate");
                    return 0;
                }
                break;
/*-------------------------------------------------------------------------
 * H5Z_FILTER_SZIP       4 , szip compression
 *-------------------------------------------------------------------------
 */
            case H5Z_FILTER_SZIP:
                if (!have_szip)
                {
                    if (name)
                        print_warning(name,"SZIP");
                    return 0;
                }
                break;
/*-------------------------------------------------------------------------
 * H5Z_FILTER_SHUFFLE    2 , shuffle the data
 *-------------------------------------------------------------------------
 */
            case H5Z_FILTER_SHUFFLE:
                if (!have_shuffle)
                {
                    if (name)
                        print_warning(name,"shuffle");
                    return 0;
                }
                break;
/*-------------------------------------------------------------------------
 * H5Z_FILTER_FLETCHER32 3 , fletcher32 checksum of EDC
 *-------------------------------------------------------------------------
 */
            case H5Z_FILTER_FLETCHER32:
                if (!have_fletcher)
                {
                    if (name)
                        print_warning(name,"fletcher32");
                    return 0;
                }
                break;
        }/*switch*/
    }/*for*/

    return 1;
}

/*-------------------------------------------------------------------------
 * Function: h5trav_print
 *
 * Purpose: Print information about the objects & links in the file
 *
 * Return: 0, -1 on error
 *
 * Programmer: Quincey Koziol, koziol@hdfgroup.org
 *
 * Date: September 6, 2007
 *
 *-------------------------------------------------------------------------
 */

int
h5trav_print(hid_t fid)
{
    trav_print_udata_t print_udata;     /* User data for traversal */
    trav_visitor_t print_visitor;       /* Visitor structure for printing objects */

    /* Init user data for printing */
    print_udata.fid = fid;

    /* Init visitor structure */
    print_visitor.visit_obj = trav_print_visit_obj;
    print_visitor.visit_lnk = trav_print_visit_lnk;
    print_visitor.udata = &print_udata;

    /* Traverse all objects in the file, visiting each object & link */
    if(traverse(fid, "/", true, true, &print_visitor) < 0)
        return -1;

    return 0;
}

static int have_request(pack_opt_t *options)
{

    if (options->all_filter || options->all_layout || options->op_tbl->nelems)
        return 1;

    return 0;

}


static int aux_inctable(pack_opttbl_t *table, int n_objs )
{
    unsigned int i;

    table->size += n_objs;
    table->objs = (pack_info_t*)realloc(table->objs, table->size * sizeof(pack_info_t));
    if (table->objs==NULL) {
        error_msg("not enough memory for options table\n");
        return -1;
    }
    for (i = table->nelems; i < table->size; i++)
    {
        init_packobject(&table->objs[i]);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function: symlink_visit_add
 *
 * Purpose: Add an symbolic link to visited data structure
 *
 * Return: 0 on success, -1 on failure
 *
 * Programmer: Neil Fortner, nfortne2@hdfgroup.org
 *             Adapted from trav_addr_add in h5trav.c by Quincey Koziol
 *
 * Date: September 5, 2008
 *
 * Modified:
 *  Jonathan Kim
 *   - Moved from h5ls.c to share among tools.  (Sep 16, 2010)
 *   - Renamed from elink_trav_add to symlink_visit_add for both soft and
 *     external links.   (May 25, 2010)
 *   - Add type parameter to distingush between soft and external link for
 *     sure, which prevent from mixing up visited link when the target names
 *     are same between the soft and external link, as code marks with the
 *     target name.  (May 25,2010)
 *
 *-------------------------------------------------------------------------
 */
herr_t
symlink_visit_add(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path)
{
    size_t  idx;         /* Index of address to use */
    void    *tmp_ptr;

    /* Allocate space if necessary */
    if(visited->nused == visited->nalloc)
    {
        visited->nalloc = MAX(1, visited->nalloc * 2);
        if(NULL == (tmp_ptr = realloc(visited->objs, visited->nalloc * sizeof(visited->objs[0]))))
            return -1;
        visited->objs = (cuco*)tmp_ptr;
    } /* end if */

    /* Append it */
    idx = visited->nused++;

    visited->objs[idx].type = type;
    visited->objs[idx].file = NULL;
    visited->objs[idx].path = NULL;

    if (type == H5L_TYPE_EXTERNAL)
    {
        if(NULL == (visited->objs[idx].file = strdup(file)))
        {
            visited->nused--;
            return -1;
        }
    }

    if(NULL == (visited->objs[idx].path = strdup(path)))
    {
        visited->nused--;
        if (visited->objs[idx].file)
            free (visited->objs[idx].file);
        return -1;
    }

    return 0;
} /* end symlink_visit_add() */


/*-------------------------------------------------------------------------
 * Function: symlink_is_visited
 *
 * Purpose: Check if an symbolic link has already been visited
 *
 * Return: true/FALSE
 *
 * Programmer: Neil Fortner, nfortne2@hdfgroup.org
 *             Adapted from trav_addr_visited in h5trav.c by Quincey Koziol
 *
 * Date: September 5, 2008
 *
 * Modified:
 *  Jonathan Kim
 *   - Moved from h5ls.c to share among tools.  (Sep 16, 2010)
 *   - Renamed from elink_trav_visited to symlink_is_visited for both soft and
 *     external links.  (May 25, 2010)
 *   - Add type parameter to distingush between soft and external link for
 *     sure, which prevent from mixing up visited link when the target names
 *     are same between the soft and external link, as code marks with the
 *     target name.  (May 25,2010)
 *
 *-------------------------------------------------------------------------
 */




hbool_t
symlink_is_visited(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path)
{
    size_t u;  /* Local index variable */

    /* Look for symlink */
    for(u = 0; u < visited->nused; u++)
    {
        /* Check for symlink values already in array */
        /* check type and path pair to distingush between symbolic links */
        if ((visited->objs[u].type == type) && !strcmp(visited->objs[u].path, path))
        {
            /* if external link, file need to be matched as well */
            if (visited->objs[u].type == H5L_TYPE_EXTERNAL)
            {
                if (!strcmp(visited->objs[u].file, file))
                    return (true);
            }
            return (true);
        }
    }
    /* Didn't find symlink */
    return(false);
} /* end symlink_is_visited() */


static
int aux_find_obj(const char* name,          /* object name from traverse list */
                 pack_opt_t *options,       /* repack options */
                 pack_info_t *obj /*OUT*/)  /* info about object to filter */
{
    char *pdest;
    int  result;
    unsigned int  i;

    for ( i=0; i<options->op_tbl->nelems; i++)
    {
        if (strcmp(options->op_tbl->objs[i].path,name)==0)
        {
            *obj =  options->op_tbl->objs[i];
            return i;
        }

        pdest  = (char*)strstr(name,options->op_tbl->objs[i].path);
        result = (int)(pdest - name);

        /* found at position 1, meaning without '/' */
        if( pdest != NULL && result==1 )
        {
            *obj =  options->op_tbl->objs[i];
            return i;
        }
    }/*i*/

    return -1;
}



obj_list_t* parse_layout(const char *str,
                         int *n_objs,
                         pack_info_t *pack,    /* info about layout needed */
                         pack_opt_t *options)
{
    obj_list_t* obj_list=NULL;
    unsigned    i;
    char        c;
    size_t      len=strlen(str);
    int         j, n, k, end_obj=-1, c_index;
    char        sobj[MAX_NC_NAME];
    char        sdim[10];
    char        slayout[10];


    memset(sdim, '\0', sizeof(sdim));
    memset(sobj, '\0', sizeof(sobj));
    memset(slayout, '\0', sizeof(slayout));

    /* check for the end of object list and number of objects */
    for ( i=0, n=0; i<len; i++)
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

    if (end_obj==-1) { /* missing : chunk all */
        options->all_layout=1;
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
    for ( j=0, k=0, n=0; j<end_obj; j++,k++)
    {
        c = str[j];
        sobj[k]=c;
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
        error_msg("in parse layout, no characters after : in <%s>\n",str);
        exit(EXIT_FAILURE);
    }

    /* get layout info */
    for ( j=end_obj+1, n=0; n<=5; j++,n++)
    {
        if (n==5)
        {
            slayout[n]='\0';  /*cut string */
            if (strcmp(slayout,"COMPA")==0)
                pack->layout=H5D_COMPACT;
            else if (strcmp(slayout,"CONTI")==0)
                pack->layout=H5D_CONTIGUOUS;
            else if (strcmp(slayout,"CHUNK")==0)
                pack->layout=H5D_CHUNKED;
            else {
                error_msg("in parse layout, not a valid layout in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            c = str[j];
            slayout[n]=c;
        }
    } /* j */


    if ( pack->layout==H5D_CHUNKED )
    {

        /*-------------------------------------------------------------------------
        * get chunk info
        *-------------------------------------------------------------------------
            */
        k=0;

        if (j>(int)len)
        {
            if (obj_list) free(obj_list);
            error_msg("in parse layout,  <%s> Chunk dimensions missing\n",str);
            exit(EXIT_FAILURE);
        }

        for ( i=j, c_index=0; i<len; i++)
        {
            c = str[i];
            sdim[k]=c;
            k++; /*increment sdim index */

            if (!isdigit(c) && c!='x'
                && c!='N' && c!='O' && c!='N' && c!='E'
                    ){
                if (obj_list) free(obj_list);
                error_msg("in parse layout, <%s> Not a valid character in <%s>\n",
                          sdim,str);
                exit(EXIT_FAILURE);
            }

            if ( c=='x' || i==len-1)
            {
                if ( c=='x') {
                    sdim[k-1]='\0';
                    k=0;
                    pack->chunk.chunk_lengths[c_index]=atoi(sdim);
                    if (pack->chunk.chunk_lengths[c_index]==0) {
                        if (obj_list) free(obj_list);
                        error_msg("in parse layout, <%s> conversion to number in <%s>\n",
                                  sdim,str);
                        exit(EXIT_FAILURE);
                    }
                    c_index++;
                }
                else if (i==len-1) { /*no more parameters */
                    sdim[k]='\0';
                    k=0;
                    if (strcmp(sdim,"NONE")==0)
                    {
                        pack->chunk.rank=-2;
                    }
                    else
                    {
                        pack->chunk.chunk_lengths[c_index]=atoi(sdim);
                        if (pack->chunk.chunk_lengths[c_index]==0){
                            if (obj_list) free(obj_list);
                            error_msg("in parse layout, <%s> conversion to number in <%s>\n",
                                      sdim,str);
                            exit(EXIT_FAILURE);
                        }
                        pack->chunk.rank=c_index+1;
                    }
                } /*if */
            } /*if c=='x' || i==len-1 */
        } /*i*/


    } /*H5D_CHUNKED*/


    return obj_list;
}

int options_add_layout( obj_list_t *obj_list,
                        int n_objs,
                        pack_info_t *pack,
                        pack_opttbl_t *table )
{
    unsigned int i, I;
    int          j, added=0, found=0;

    /* increase the size of the collection by N_OBJS if necessary */
    if (table->nelems+n_objs >= table->size)
    {
        if (aux_inctable(table,n_objs)<0)
            return -1;
    }

    /* search if this object is already in the table; "path" is the key */
    if (table->nelems>0)
    {
        /* go tru the supplied list of names */
        for (j = 0; j < n_objs; j++)
        {
            /* linear table search */
            for (i = 0; i < table->nelems; i++)
            {
                /*already on the table */
                if (strcmp(obj_list[j].obj,table->objs[i].path)==0)
                {
                    /* already chunk info inserted for this one; exit */
                    if (table->objs[i].chunk.rank>0)
                    {
                        error_msg("chunk information already inserted for <%s>\n",obj_list[j].obj);
                        exit(EXIT_FAILURE);
                    }
                        /* insert the layout info */
                    else
                    {
                        aux_tblinsert_layout(table,i,pack);
                        found=1;
                        break;
                    }
                } /* if */
            } /* i */

            if (found==0)
            {
                /* keep the grow in a temp var */
                I = table->nelems + added;
                added++;
                strcpy(table->objs[I].path,obj_list[j].obj);
                aux_tblinsert_layout(table,I,pack);
            }
                /* cases where we have an already inserted name but there is a new name also
                example:
                -f dset1:GZIP=1 -l dset1,dset2:CHUNK=20x20
                dset1 is already inserted, but dset2 must also be
                */
            else if (found==1 && strcmp(obj_list[j].obj,table->objs[i].path)!=0)
            {
                /* keep the grow in a temp var */
                I = table->nelems + added;
                added++;
                strcpy(table->objs[I].path,obj_list[j].obj);
                aux_tblinsert_layout(table,I,pack);
            }
        } /* j */
    }

        /* first time insertion */
    else
    {
        /* go tru the supplied list of names */
        for (j = 0; j < n_objs; j++)
        {
            I = table->nelems + added;
            added++;
            strcpy(table->objs[I].path,obj_list[j].obj);
            aux_tblinsert_layout(table,I,pack);

        }
    }

    table->nelems+= added;

    return 0;
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

    /* name was on input */
    if (idx>=0)
    {


        /* applying to all objects */
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
        else
        {
            tmp.layout = options->op_tbl->objs[idx].layout;
            switch (tmp.layout)
            {
                case H5D_CHUNKED:
                    tmp.chunk.rank = options->op_tbl->objs[idx].chunk.rank;
                    for ( i=0; i<tmp.chunk.rank; i++)
                        tmp.chunk.chunk_lengths[i]=options->op_tbl->objs[idx].chunk.chunk_lengths[i];
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

        /* applying to all objects */
        if (options->all_filter)
        {
            /* assign the global filter */
            tmp.nfilters=1;
            tmp.filter[0]=options->filter_g[0];
        } /* if all */
        else
        {
            tmp.nfilters=options->op_tbl->objs[idx].nfilters;
            for ( i=0; i<tmp.nfilters; i++)
            {
                tmp.filter[i] = options->op_tbl->objs[idx].filter[i];
            }
        }


    } /* if idx */


        /* no input name */

    else
    {

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

                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_SZIP
                 * szip has the format SZIP=<pixels per block,coding>
                 * pixels per block is a even number in 2-32 and coding method is 'EC' or 'NN'
                 * example SZIP=8,NN
                 *-------------------------------------------------------------------------
                 */
                if (strcmp(scomp,"SZIP")==0)
                {
                    l=-1; /* mask index check */
                    for ( m=0,u=i+1; u<len; u++,m++)
                    {
                        if (str[u]==',')
                        {
                            stype[m]='\0'; /* end digit of szip */
                            l=0;  /* start EC or NN search */
                            u++;  /* skip ',' */
                        }
                        c = str[u];
                        if (!isdigit(c) && l==-1){
                            if (obj_list) free(obj_list);
                            error_msg("compression parameter not digit in <%s>\n",str);
                            exit(EXIT_FAILURE);
                        }
                        if (l==-1)
                            stype[m]=c;
                        else
                        {
                            smask[l]=c;
                            l++;
                            if (l==2)
                            {
                                smask[l]='\0';
                                i=len-1; /* end */
                                (*n_objs)--; /* we counted an extra ',' */
                                if (strcmp(smask,"NN")==0)
                                    filt->cd_values[j++]=H5_SZIP_NN_OPTION_MASK;
                                else if (strcmp(smask,"EC")==0)
                                    filt->cd_values[j++]=H5_SZIP_EC_OPTION_MASK;
                                else
                                {
                                    error_msg("szip mask must be 'NN' or 'EC' \n");
                                    exit(EXIT_FAILURE);
                                }


                            }
                        }

                    }  /* u */
                } /*if */

                    /*-------------------------------------------------------------------------
                    * H5Z_FILTER_SCALEOFFSET
                    * scaleoffset has the format SOFF=<scale_factor,scale_type>
                    * scale_type can be
                    *   integer datatype, H5Z_SO_INT (IN)
                    *   float datatype using D-scaling method, H5Z_SO_FLOAT_DSCALE  (DS)
                    *   float datatype using E-scaling method, H5Z_SO_FLOAT_ESCALE  (ES) , not yet implemented
                    * for integer datatypes, scale_factor denotes Minimum Bits
                    * for float datatypes, scale_factor denotes decimal scale factor
                    *  examples
                    *  SOFF=31,IN
                    *  SOFF=3,DF
                    *-------------------------------------------------------------------------
                  */

                else if (strcmp(scomp,"SOFF")==0)
                {
                    l=-1; /* mask index check */
                    for ( m=0,u=i+1; u<len; u++,m++)
                    {
                        if (str[u]==',')
                        {
                            stype[m]='\0'; /* end digit */
                            l=0;  /* start 'IN' , 'DS', or 'ES' search */
                            u++;  /* skip ',' */
                        }
                        c = str[u];
                        if (!isdigit(c) && l==-1){
                            if (obj_list) free(obj_list);
                            error_msg("compression parameter is not a digit in <%s>\n",str);
                            exit(EXIT_FAILURE);
                        }
                        if (l==-1)
                            stype[m]=c;
                        else
                        {
                            smask[l]=c;
                            l++;
                            if (l==2)
                            {
                                smask[l]='\0';
                                i=len-1; /* end */
                                (*n_objs)--; /* we counted an extra ',' */
                                if (strcmp(smask,"IN")==0)
                                    filt->cd_values[j++]=H5Z_SO_INT;
                                else if (strcmp(smask,"DS")==H5Z_SO_FLOAT_DSCALE)
                                    filt->cd_values[j++]=H5Z_SO_FLOAT_DSCALE;
                                else
                                {
                                    error_msg("scale type must be 'IN' or 'DS' \n");
                                    exit(EXIT_FAILURE);
                                }

                            }
                        }

                    }  /* u */
                } /*if */


                    /*-------------------------------------------------------------------------
                     * all other filters
                     *-------------------------------------------------------------------------
                     */

                else
                {
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
                } /*if */



                filt->cd_values[j++]=atoi(stype);
                i+=m; /* jump */
            }
            else if (i==len-1)
            { /*no more parameters */
                scomp[k+1]='\0';
                no_param=1;
            }

            /*-------------------------------------------------------------------------
             * translate from string to filter symbol
             *-------------------------------------------------------------------------
             */

            /*-------------------------------------------------------------------------
             * H5Z_FILTER_NONE
             *-------------------------------------------------------------------------
             */
            if (strcmp(scomp,"NONE")==0)
            {
                filt->filtn=H5Z_FILTER_NONE;
                filt->cd_nelmts = 0;
            }

                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_DEFLATE
                 *-------------------------------------------------------------------------
                 */
            else if (strcmp(scomp,"GZIP")==0)
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

                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_SZIP
                 *-------------------------------------------------------------------------
                 */
            else if (strcmp(scomp,"SZIP")==0)
            {
                filt->filtn=H5Z_FILTER_SZIP;
                filt->cd_nelmts = 2;
                if (no_param)
                { /*no more parameters, SZIP must have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("missing compression parameter in <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }

                /*-------------------------------------------------------------------------
                * H5Z_FILTER_SHUFFLE
                *-------------------------------------------------------------------------
                */
            else if (strcmp(scomp,"SHUF")==0)
            {
                filt->filtn=H5Z_FILTER_SHUFFLE;
                filt->cd_nelmts = 0;
                if (m>0)
                { /*shuffle does not have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("extra parameter in SHUF <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }
                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_FLETCHER32
                 *-------------------------------------------------------------------------
                 */
            else if (strcmp(scomp,"FLET")==0)
            {
                filt->filtn=H5Z_FILTER_FLETCHER32;
                filt->cd_nelmts = 0;
                if (m>0)
                { /*shuffle does not have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("extra parameter in FLET <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }
                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_NBIT
                 *-------------------------------------------------------------------------
                 */
            else if (strcmp(scomp,"NBIT")==0)
            {
                filt->filtn=H5Z_FILTER_NBIT;
                filt->cd_nelmts = 0;
                if (m>0)
                { /*nbit does not have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("extra parameter in NBIT <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }
                /*-------------------------------------------------------------------------
                 * H5Z_FILTER_SCALEOFFSET
                 *-------------------------------------------------------------------------
                 */
            else if (strcmp(scomp,"SOFF")==0)
            {
                filt->filtn=H5Z_FILTER_SCALEOFFSET;
                filt->cd_nelmts = 2;
                if (no_param)
                { /*no more parameters, SOFF must have parameter */
                    if (obj_list) free(obj_list);
                    error_msg("missing compression parameter in <%s>\n",str);
                    exit(EXIT_FAILURE);
                }
            }
            else {
                if (obj_list) free(obj_list);
                error_msg("invalid filter type in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
        }
    } /*i*/

    /*-------------------------------------------------------------------------
     * check valid parameters
     *-------------------------------------------------------------------------
     */

    switch (filt->filtn)
    {

        /*-------------------------------------------------------------------------
         * H5Z_FILTER_DEFLATE
         *-------------------------------------------------------------------------
         */

        case H5Z_FILTER_DEFLATE:
            if (filt->cd_values[0]>9 )
            {
                if (obj_list) free(obj_list);
                error_msg("invalid compression parameter in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
            break;

            /*-------------------------------------------------------------------------
             * H5Z_FILTER_SZIP
             *-------------------------------------------------------------------------
             */

        case H5Z_FILTER_SZIP:
            pixels_per_block=filt->cd_values[0];
            if ((pixels_per_block%2)==1)
            {
                if (obj_list) free(obj_list);
                error_msg("pixels_per_block is not even in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
            if (pixels_per_block>H5_SZIP_MAX_PIXELS_PER_BLOCK)
            {
                if (obj_list) free(obj_list);
                error_msg("pixels_per_block is too large in <%s>\n",str);
                exit(EXIT_FAILURE);
            }
            if ( (strcmp(smask,"NN")!=0) && (strcmp(smask,"EC")!=0) )
            {
                if (obj_list) free(obj_list);
                error_msg("szip mask must be 'NN' or 'EC' \n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;


    };

    return obj_list;
}




/*-------------------------------------------------------------------------
* Function: h5repack_end
*
* Purpose: free options table
*
*-------------------------------------------------------------------------
*/

int options_add_filter(obj_list_t *obj_list,
                       int n_objs,
                       filter_info_t filt,
                       pack_opttbl_t *table )
{

    unsigned int i, I;
    int          j, added=0, found=0;

    /* increase the size of the collection by N_OBJS if necessary */
    if (table->nelems+n_objs >= table->size)
    {
        if (aux_inctable(table,n_objs)<0)
            return -1;
    }

    /* search if this object is already in the table; "path" is the key */
    if (table->nelems>0)
    {
        /* go tru the supplied list of names */
        for (j = 0; j < n_objs; j++)
        {
            /* linear table search */
            for (i = 0; i < table->nelems; i++)
            {
                /*already on the table */
                if (strcmp(obj_list[j].obj,table->objs[i].path)==0)
                {
                    /* insert */
                    aux_tblinsert_filter(table,i,filt);
                    found=1;
                    break;
                } /* if */
            } /* i */

            if (found==0)
            {
                /* keep the grow in a temp var */
                I = table->nelems + added;
                added++;
                strcpy(table->objs[I].path,obj_list[j].obj);
                aux_tblinsert_filter(table,I,filt);
            }
                /* cases where we have an already inserted name but there is a new name also
                example:
                -l dset1:CHUNK=20x20 -f dset1,dset2:GZIP=1
                dset1 is already inserted, but dset2 must also be
                */
            else if (found==1 && strcmp(obj_list[j].obj,table->objs[i].path)!=0)
            {
                /* keep the grow in a temp var */
                I = table->nelems + added;
                added++;
                strcpy(table->objs[I].path,obj_list[j].obj);
                aux_tblinsert_filter(table,I,filt);
            }
        } /* j */
    }

        /* first time insertion */
    else
    {
        /* go tru the supplied list of names */
        for (j = 0; j < n_objs; j++)
        {
            I = table->nelems + added;
            added++;
            strcpy(table->objs[I].path,obj_list[j].obj);
            aux_tblinsert_filter(table,I,filt);
        }
    }

    table->nelems+= added;

    return 0;
}


int h5repack_end  (pack_opt_t *options)
{
    return options_table_free(options->op_tbl);
}


/*-------------------------------------------------------------------------
* Function: h5repack_addfilter
*
* Purpose: add a compression -f option to table
*   Example: -f dset:GZIP=6
*
* Return: 0, ok, -1, fail
*
*-------------------------------------------------------------------------
*/
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
    else
        options_add_filter(obj_list, n_objs, filter, options->op_tbl);

    free(obj_list);
    return 0;
}


/*-------------------------------------------------------------------------
* Function: h5repack_addlayout
*
* Purpose: add a layout option
*
* Return: 0, ok, -1, fail
*
*-------------------------------------------------------------------------
*/


int h5repack_addlayout(const char* str,
                       pack_opt_t *options)
{

    obj_list_t  *obj_list=NULL;     /*one object list for the -t and -c option entry */
    int         n_objs;             /*number of objects in the current -t or -c option entry */
    pack_info_t pack;               /*info about layout to extract from parse */
    int         j;

    init_packobject(&pack);

    if (options->all_layout==1){
        error_msg("invalid layout input: 'all' option \
                            is present with other objects <%s>\n",str);
        return -1;
    }

    /* parse the layout option */
    obj_list=parse_layout(str,&n_objs,&pack,options);
    if (obj_list==NULL)
        return -1;

    /* set global layout option */
    if (options->all_layout==1 )
    {
        options->layout_g=pack.layout;
        if (pack.layout==H5D_CHUNKED)
        {
            /* -2 means the NONE option, remove chunking
            and set the global layout to contiguous */
            if (pack.chunk.rank==-2)
            {
                options->layout_g = H5D_CONTIGUOUS;
            }
                /* otherwise set the global chunking type */
            else
            {
                options->chunk_g.rank=pack.chunk.rank;
                for (j = 0; j < pack.chunk.rank; j++)
                    options->chunk_g.chunk_lengths[j] = pack.chunk.chunk_lengths[j];
            }
        }
    }

    if (options->all_layout==0)
        options_add_layout(obj_list,
                           n_objs,
                           &pack,
                           options->op_tbl);

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


/*-------------------------------------------------------------------------
* Function: named_datatype_free
*
* Purpose: Frees the stack of named datatypes.
*
* Programmer: Neil Fortner
*
* Date: April 14, 2009
*
*-------------------------------------------------------------------------
*/
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

    /*-------------------------------------------------------------------------
     * initialize the assigment object
     *-------------------------------------------------------------------------
     */
    init_packobject(&obj);

    /*-------------------------------------------------------------------------
     * find options
     *-------------------------------------------------------------------------
     */
    if (aux_assign_obj(name,options,&obj)==0)
        return 0;

    /* get information about input filters */
    if ((nfilters = H5Pget_nfilters(dcpl_id))<0)
        return -1;

    /*-------------------------------------------------------------------------
     * check if we have filters in the pipeline
     * we want to replace them with the input filters
     * only remove if we are inserting new ones
     *-------------------------------------------------------------------------
     */
    if (nfilters && obj.nfilters )
    {
        *has_filter = 1;
        if (H5Premove_filter(dcpl_id,H5Z_FILTER_ALL)<0)
            return -1;
    }

    /*-------------------------------------------------------------------------
     * check if there is an existent chunk
     * read it only if there is not a requested layout
     *-------------------------------------------------------------------------
     */
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

    /*-------------------------------------------------------------------------
    * the type of filter and additional parameter
    * type can be one of the filters
    * H5Z_FILTER_NONE        0 , uncompress if compressed
    * H5Z_FILTER_DEFLATE     1 , deflation like gzip
    * H5Z_FILTER_SHUFFLE     2 , shuffle the data
    * H5Z_FILTER_FLETCHER32  3 , fletcher32 checksum of EDC
    * H5Z_FILTER_SZIP        4 , szip compression
    * H5Z_FILTER_NBIT        5 , nbit compression
    * H5Z_FILTER_SCALEOFFSET 6 , scaleoffset compression
    *-------------------------------------------------------------------------
    */

    if (obj.nfilters)
    {

        /*-------------------------------------------------------------------------
         * filters require CHUNK layout; if we do not have one define a default
         *-------------------------------------------------------------------------
         */
        if (obj.layout==-1)
        {

            /* stripmine info */
            hsize_t sm_size[H5S_MAX_RANK]; /*stripmine size */
            hsize_t sm_nbytes;             /*bytes per stripmine */

            obj.chunk.rank = rank;

            /*
            * determine the strip mine size. The strip mine is
            * a hyperslab whose size is manageable.
            */



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

                    /*-------------------------------------------------------------------------
                     * H5Z_FILTER_DEFLATE       1 , deflation like gzip
                     *-------------------------------------------------------------------------
                     */
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

                    /*-------------------------------------------------------------------------
                     * H5Z_FILTER_SZIP       4 , szip compression
                     *-------------------------------------------------------------------------
                     */
                case H5Z_FILTER_SZIP:
                {
                    unsigned  options_mask;
                    unsigned  pixels_per_block;

                    options_mask     = obj.filter[i].cd_values[0];
                    pixels_per_block = obj.filter[i].cd_values[1];

                    /* set up for szip data */
                    if(H5Pset_chunk(dcpl_id,obj.chunk.rank,obj.chunk.chunk_lengths)<0)
                        return -1;
                    if (H5Pset_szip(dcpl_id,options_mask,pixels_per_block)<0)
                        return -1;

                }
                    break;

                    /*-------------------------------------------------------------------------
                     * H5Z_FILTER_SHUFFLE    2 , shuffle the data
                     *-------------------------------------------------------------------------
                     */
                case H5Z_FILTER_SHUFFLE:
                    if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                        return -1;
                    if (H5Pset_shuffle(dcpl_id)<0)
                        return -1;
                    break;

                    /*-------------------------------------------------------------------------
                     * H5Z_FILTER_FLETCHER32 3 , fletcher32 checksum of EDC
                     *-------------------------------------------------------------------------
                     */
                case H5Z_FILTER_FLETCHER32:
                    if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                        return -1;
                    if (H5Pset_fletcher32(dcpl_id)<0)
                        return -1;
                    break;
                    /*----------- -------------------------------------------------------------
                     * H5Z_FILTER_NBIT , NBIT compression
                     *-------------------------------------------------------------------------
                     */
                case H5Z_FILTER_NBIT:
                    if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                        return -1;
                    if (H5Pset_nbit(dcpl_id)<0)
                        return -1;
                    break;
                    /*----------- -------------------------------------------------------------
                     * H5Z_FILTER_SCALEOFFSET , scale+offset compression
                     *-------------------------------------------------------------------------
                     */

                case H5Z_FILTER_SCALEOFFSET:
                {
                    H5Z_SO_scale_type_t scale_type;
                    int                 scale_factor;

                    scale_type   = (H5Z_SO_scale_type_t)obj.filter[i].cd_values[0];
                    scale_factor = obj.filter[i].cd_values[1];

                    if(H5Pset_chunk(dcpl_id, obj.chunk.rank, obj.chunk.chunk_lengths)<0)
                        return -1;
                    if (H5Pset_scaleoffset(dcpl_id,scale_type,scale_factor)<0)
                        return -1;
                }
                    break;
            } /* switch */
        }/*i*/

    }
    /*obj.nfilters*/

    /*-------------------------------------------------------------------------
    * layout
    *-------------------------------------------------------------------------
    */

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
                        if(options->verbose)
                            printf("object <%s> reference created to <%s>\n", name, refname);
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
                        if(options->verbose)
                            printf("object <%s> region reference created to <%s>\n", name, refname);
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


        if(options->verbose)
            printf(FORMAT_OBJ_ATTR, "attr", name);

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

                /*-------------------------------------------------------------------------
                 * check if the dataset creation property list has filters that
                 * are not registered in the current configuration
                 * 1) the external filters GZIP and SZIP might not be available
                 * 2) the internal filters might be turned off
                 *-------------------------------------------------------------------------
                 */
                if(h5tools_canreadf(NULL, dcpl_id) == 1) {
                    /*-------------------------------------------------------------------------
                    * test for a valid output dataset
                    *-------------------------------------------------------------------------
                    */
                    dset_out = FAIL;

                    /*-------------------------------------------------------------------------
                    * object references are a special case
                    * we cannot just copy the buffers, but instead we recreate the reference
                    *-------------------------------------------------------------------------
                    */
                    if(H5Tequal(mtype_id, H5T_STD_REF_OBJ)) {
                        hid_t            refobj_id;
                        hobj_ref_t       *refbuf = NULL; /* buffer for object references */
                        hobj_ref_t       *buf = NULL;
                        const char*      refname;
                        unsigned         u;

                        /*-------------------------------------------------------------------------
                        * read to memory
                        *-------------------------------------------------------------------------
                        */
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
                                    if(options->verbose)
                                    {


                                        printf(FORMAT_OBJ,"dset",travt->objs[i].name );
                                        printf("object <%s> object reference created to <%s>\n",
                                               travt->objs[i].name,
                                               refname);
                                    }
                                } /*refname*/
                                H5Oclose(refobj_id);
                            } /* u */
                        } /*nelmts*/

                        /*-------------------------------------------------------------------------
                        * create/write dataset/close
                        *-------------------------------------------------------------------------
                        */
                        if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, mtype_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                            goto error;
                        if(nelmts)
                            if(H5Dwrite(dset_out, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, refbuf) < 0)
                                goto error;

                        if(buf)
                            free(buf);
                        if(refbuf)
                            free(refbuf);

                        /*------------------------------------------------------
                         * copy attrs
                         *----------------------------------------------------*/
                        if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                            goto error;
                    } /*H5T_STD_REF_OBJ*/

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
                        unsigned         u;

                        /*-------------------------------------------------------------------------
                        * read input to memory
                        *-------------------------------------------------------------------------
                        */
                        if(nelmts) {
                            buf = (hdset_reg_ref_t *)malloc((unsigned)(nelmts * msize));
                            if(buf == NULL) {
                                printf("cannot read into memory\n");
                                goto error;
                            } /* end if */
                            if(H5Dread(dset_in, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0)
                                goto error;

                            /*-------------------------------------------------------------------------
                            * create output
                            *-------------------------------------------------------------------------
                            */
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
                                    if(options->verbose)
                                    {



                                        printf(FORMAT_OBJ,"dset",travt->objs[i].name );
                                        printf("object <%s> region reference created to <%s>\n",
                                               travt->objs[i].name,
                                               refname);
                                    }
                                } /*refname*/
                                H5Oclose(refobj_id);
                            } /* u */
                        } /*nelmts*/

                        /*-------------------------------------------------------------------------
                        * create/write dataset/close
                        *-------------------------------------------------------------------------
                        */
                        if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, mtype_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                            goto error;
                        if(nelmts)
                            if(H5Dwrite(dset_out, mtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, refbuf) < 0)
                                goto error;

                        if(buf)
                            free(buf);
                        if(refbuf)
                            free(refbuf);

                        /*-----------------------------------------------------
                         * copy attrs
                         *----------------------------------------------------*/
                        if(copy_attr(dset_in, dset_out, &named_dt_head, travt, options) < 0)
                            goto error;
                    } /* H5T_STD_REF_DSETREG */
                        /*-------------------------------------------------------------------------
                        * not references, open previously created object in 1st traversal
                        *-------------------------------------------------------------------------
                        */
                    else {
                        if((dset_out = H5Dopen2(fidout, travt->objs[i].name, H5P_DEFAULT)) < 0)
                            goto error;
                    } /* end else */

                    /*-------------------------------------------------------------------------
                    * copy referenced objects in attributes
                    *-------------------------------------------------------------------------
                    */
                    if(copy_refs_attr(dset_in, dset_out, options, travt, fidout) < 0)
                        goto error;

                    /*-------------------------------------------------------------------------
                    * check for hard links
                    *-------------------------------------------------------------------------
                    */
                    if(travt->objs[i].nlinks)
                        for(j = 0; j < travt->objs[i].nlinks; j++)
                            H5Lcreate_hard(fidout, travt->objs[i].name, H5L_SAME_LOC, travt->objs[i].links[j].new_name, H5P_DEFAULT, H5P_DEFAULT);

                    if(H5Dclose(dset_out) < 0)
                        goto error;
                } /*can_read*/

                /*-------------------------------------------------------------------------
                * close
                *-------------------------------------------------------------------------
                */
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

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_NAMED_DATATYPE
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_NAMED_DATATYPE:
                if((type_in = H5Topen2(fidin, travt->objs[i].name, H5P_DEFAULT)) < 0)
                    goto error;
                if(H5Tclose(type_in) < 0)
                    goto error;
                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_LINK
                *-------------------------------------------------------------------------
                */
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

    /* Finalize (link) the stack of named datatypes (if any)
     * This function is paired with copy_named_datatype() which is called
     * in copy_attr(), so need to free.
     */
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


static int check_options(pack_opt_t *options)
{
    unsigned int   i;
    int            k, j, has_cp=0, has_ck=0;
    char           slayout[30];

    /*-------------------------------------------------------------------------
    * objects to layout
    *-------------------------------------------------------------------------
    */
    if (options->verbose && have_request(options) /* only print if requested */)
    {
        printf("Objects to modify layout are...\n");
        if (options->all_layout==1)
        {
            switch (options->layout_g)
            {
                case H5D_COMPACT:
                    strcpy(slayout,"compact");
                    break;
                case H5D_CONTIGUOUS:
                    strcpy(slayout,"contiguous");
                    break;
                case H5D_CHUNKED:
                    strcpy(slayout,"chunked");
                    break;
                case H5D_LAYOUT_ERROR:
                case H5D_NLAYOUTS:
                    error_msg("invalid layout\n");
                    return -1;
                default:
                    strcpy(slayout,"invalid layout\n");
                    return -1;
            }
            printf(" Apply %s layout to all\n", slayout);
            if (H5D_CHUNKED==options->layout_g)
            {
                printf("with dimension [");
                for ( j = 0; j < options->chunk_g.rank; j++)
                    printf("%d ",(int)options->chunk_g.chunk_lengths[j]);
                printf("]\n");
            }
        }
    }/* verbose */

    for ( i = 0; i < options->op_tbl->nelems; i++)
    {
        char* name=options->op_tbl->objs[i].path;

        if (options->op_tbl->objs[i].chunk.rank>0)
        {
            if (options->verbose){
                printf(" <%s> with chunk size ",name);
                for ( k = 0; k < options->op_tbl->objs[i].chunk.rank; k++)
                    printf("%d ",(int)options->op_tbl->objs[i].chunk.chunk_lengths[k]);
                printf("\n");
            }
            has_ck=1;
        }
        else if (options->op_tbl->objs[i].chunk.rank==-2)
        {
            if (options->verbose)
                printf(" <%s> %s\n",name,"NONE (contigous)");
            has_ck=1;
        }
    }

    if (options->all_layout==1 && has_ck)
    {
        error_msg("invalid chunking input: 'all' option\
                            is present with other objects\n");
        return -1;
    }

    /*-------------------------------------------------------------------------
    * objects to filter
    *-------------------------------------------------------------------------
    */

    if (options->verbose && have_request(options) /* only print if requested */)
    {
        printf("Objects to apply filter are...\n");
        if (options->all_filter==1)
        {

            for (k = 0; k < options->n_filter_g; k++ )
            {
                H5Z_filter_t filtn=options->filter_g[k].filtn;
                switch (filtn)
                {
                    case H5Z_FILTER_NONE:
                        printf(" Uncompress all\n");
                        break;
                    case H5Z_FILTER_SHUFFLE:
                    case H5Z_FILTER_FLETCHER32:
                        printf(" All with %s\n",get_sfilter(filtn));
                        break;
                    case H5Z_FILTER_SZIP:
                    case H5Z_FILTER_DEFLATE:
                        printf(" All with %s, parameter %d\n",
                               get_sfilter(filtn),
                               options->filter_g[k].cd_values[0]);
                        break;
                    default:
                        break;
                } /* k */
            };
        }
    } /* verbose */

    for ( i = 0; i < options->op_tbl->nelems; i++)
    {
        pack_info_t pack  = options->op_tbl->objs[i];
        char*       name  = pack.path;

        for ( j=0; j<pack.nfilters; j++)
        {
            if (options->verbose)
            {
                printf(" <%s> with %s filter\n",
                       name,
                       get_sfilter(pack.filter[j].filtn));
            }

            has_cp=1;

        } /* j */
    } /* i */

    if (options->all_filter==1 && has_cp)
    {
        error_msg("invalid compression input: 'all' option\
                            is present with other objects\n");
        return -1;
    }

    /*-------------------------------------------------------------------------
    * check options for the latest format
    *-------------------------------------------------------------------------
    */

    if (options->grp_compact < 0)
    {
        error_msg("invalid maximum number of links to store as header messages\n");
        return -1;
    }
    if (options->grp_indexed < 0)
    {
        error_msg("invalid minimum number of links to store in the indexed format\n");
        return -1;
    }
    if (options->grp_indexed > options->grp_compact)
    {
        error_msg("minimum indexed size is greater than the maximum compact size\n");
        return -1;
    }
    for (i=0; i<8; i++)
    {
        if (options->msg_size[i]<0)
        {
            error_msg("invalid shared message size\n");
            return -1;
        }
    }


    /*--------------------------------------------------------------------------------
    * verify new user userblock options; file name must be present
    *---------------------------------------------------------------------------------
    */
    if ( options->ublock_filename != NULL && options->ublock_size == 0 )
    {
        if ( options->verbose )
        {
            printf("Warning: user block size missing for file %s. Assigning a default size of 1024...\n",
                   options->ublock_filename);
            options->ublock_size = 1024;
        }
    }

    if ( options->ublock_filename == NULL && options->ublock_size != 0 )
    {
        error_msg("file name missing for user block\n",
                  options->ublock_filename);
        return -1;
    }


    /*--------------------------------------------------------------------------------
    * verify alignment options; threshold is zero default but alignment not
    *---------------------------------------------------------------------------------
    */

    if ( options->alignment == 0 && options->threshold != 0 )
    {
        error_msg("alignment for H5Pset_alignment missing\n");
        return -1;
    }

    return 0;
}


static int check_objects(const char* fname,
                         pack_opt_t *options)
{
    hid_t         fid;
    unsigned int  i;
    trav_table_t  *travt = NULL;

    /* nothing to do */
    if(options->op_tbl->nelems == 0)
        return 0;

    /*-------------------------------------------------------------------------
    * open the file
    *-------------------------------------------------------------------------
    */
    if((fid = h5tools_fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT, NULL, NULL, 0)) < 0)
    {
        printf("<%s>: %s\n", fname, H5FOPENERROR );
        return -1;
    }

    /*-------------------------------------------------------------------------
    * get the list of objects in the file
    *-------------------------------------------------------------------------
    */

    /* init table */
    trav_table_init(&travt);

    /* get the list of objects in the file */
    if(h5trav_gettable(fid, travt) < 0)
        goto out;

    /*-------------------------------------------------------------------------
    * compare with user supplied list
    *-------------------------------------------------------------------------
    */

    if(options->verbose)
        printf("Opening file <%s>. Searching for objects to modify...\n", fname);

    for(i = 0; i < options->op_tbl->nelems; i++)
    {
        char* name=options->op_tbl->objs[i].path;
        if(options->verbose)
            printf(" <%s>",name);

        /* the input object names are present in the file and are valid */
        if(h5trav_getindext(name, travt) < 0)
        {
            error_msg("%s Could not find <%s> in file <%s>. Exiting...\n",
                      (options->verbose?"\n":""),name,fname);
            goto out;
        }
        if(options->verbose)
            printf("...Found\n");

        /* check for extra filter conditions */
        switch(options->op_tbl->objs[i].filter->filtn)
        {
            /* chunk size must be smaller than pixels per block */
            case H5Z_FILTER_SZIP:
            {
                int      j;
                hsize_t  csize = 1;
                unsigned ppb = options->op_tbl->objs[i].filter->cd_values[0];
                hsize_t  dims[H5S_MAX_RANK];
                int      rank;
                hid_t    did;
                hid_t    sid;

                if(options->op_tbl->objs[i].chunk.rank > 0) {
                    rank = options->op_tbl->objs[i].chunk.rank;
                    for(j = 0; j < rank; j++)
                        csize *= options->op_tbl->objs[i].chunk.chunk_lengths[j];
                }
                else {
                    if((did = H5Dopen2(fid, name, H5P_DEFAULT)) < 0)
                        goto out;
                    if((sid = H5Dget_space(did)) < 0)
                        goto out;
                    if((rank = H5Sget_simple_extent_ndims(sid)) < 0)
                        goto out;
                    memset(dims, 0, sizeof dims);
                    if(H5Sget_simple_extent_dims(sid, dims, NULL) < 0)
                        goto out;
                    for(j = 0; j < rank; j++)
                        csize *= dims[j];
                    if(H5Sclose(sid) < 0)
                        goto out;
                    if(H5Dclose(did) < 0)
                        goto out;
                }

                if (csize < ppb ) {
                    printf(" <warning: SZIP settins, chunk size is smaller than pixels per block>\n");
                    goto out;
                }
            }
                break;
            default:
                break;
        }
    } /* i */

    /*-------------------------------------------------------------------------
    * close
    *-------------------------------------------------------------------------
    */
    H5Fclose(fid);
    trav_table_free(travt);
    return 0;

    out:
    H5Fclose(fid);
    trav_table_free(travt);
    return -1;
}










/* module-scoped variables */
static int  has_i_o = 0;
const char  *infile  = NULL;
const char  *outfile = NULL;


/*
 * Command-line options: The user can specify short or long-named
 * parameters.
 */
static const char *s_opts = "hVvf:l:m:e:nLc:d:s:u:b:t:a:i:o:";




/*-------------------------------------------------------------------------
 * Function: main
 *
 * Purpose: h5repack main program
 *
 * Return: Success: EXIT_SUCCESS(0)
 *
 * Failure: EXIT_FAILURE(1)
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: May 9, 2003
 *
 * Comments:
 *
 * Modifications:
 *  July 2004: Introduced the extra EC or NN option for SZIP
 *  October 2006: Added a new switch -n, that allows to write the dataset
 *                using a native type. The default to write is the file type.
 *
 * Modification:
 *   Peter Cao, June 13, 2007
 *    Add "-L, --latest" option to pack a file with the latest file format
 *   PVN, November 19, 2007
 *    adopted the syntax h5repack [OPTIONS]  file1 file2
 *   PVN, November 28, 2007
 *    added support for multiple global filters
 *   PVN, May 16, 2008
 *    added  backward compatibility for -i infile -o outfile
 *   PVN, August 20, 2008
 *    add a user block to repacked file (switches -u -b)
 *   PVN, August 28, 2008
 *    add options to set alignment (H5Pset_alignment) (switches -t -a)
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function: read_info
 *
 * Purpose: read comp and chunk options from a file
 *
 * Return: void, exit on error
 *
 * Programmer: pvn@ncsa.uiuc.edu
 *
 * Date: September, 22, 2003
 *
 *-------------------------------------------------------------------------
 */

static
void read_info(const char *filename,
               pack_opt_t *options)
{

    char stype[10];
    char comp_info[1024];
    FILE *fp;
    char c;
    int  i, rc=1;
    char  *srcdir = getenv("srcdir"); /* the source directory */
    char  data_file[512]="";          /* buffer to hold name of existing file */

    /* compose the name of the file to open, using the srcdir, if appropriate */
    if (srcdir){
        strcpy(data_file,srcdir);
        strcat(data_file,"/");
    }
    strcat(data_file,filename);


    if ((fp = fopen(data_file, "r")) == (FILE *)NULL) {
        error_msg("cannot open options file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    /* cycle until end of file reached */
    while( 1 )
    {
        rc=fscanf(fp, "%s", stype);
        if (rc==-1)
            break;

        /*-------------------------------------------------------------------------
         * filter
         *-------------------------------------------------------------------------
         */
        if (strcmp(stype,"-f") == 0) {

            /* find begining of info */
            i=0; c='0';
            while( c!=' ' )
            {
                fscanf(fp, "%c", &c);
                if (feof(fp)) break;
            }
            c='0';
            /* go until end */
            while( c!=' ' )
            {
                fscanf(fp, "%c", &c);
                comp_info[i]=c;
                i++;
                if (feof(fp)) break;
                if (c==10 /*eol*/) break;
            }
            comp_info[i-1]='\0'; /*cut the last " */

            if (h5repack_addfilter(comp_info,options)==-1){
                error_msg("could not add compression option\n");
                exit(EXIT_FAILURE);
            }
        }
            /*-------------------------------------------------------------------------
            * layout
            *-------------------------------------------------------------------------
            */
        else if (strcmp(stype,"-l") == 0) {

            /* find begining of info */
            i=0; c='0';
            while( c!=' ' )
            {
                fscanf(fp, "%c", &c);
                if (feof(fp)) break;
            }
            c='0';
            /* go until end */
            while( c!=' ' )
            {
                fscanf(fp, "%c", &c);
                comp_info[i]=c;
                i++;
                if (feof(fp)) break;
                if (c==10 /*eol*/) break;
            }
            comp_info[i-1]='\0'; /*cut the last " */

            if (h5repack_addlayout(comp_info,options)==-1){
                error_msg("could not add chunck option\n");
                exit(EXIT_FAILURE);
            }
        }
            /*-------------------------------------------------------------------------
            * not valid
            *-------------------------------------------------------------------------
            */
        else {
            error_msg("bad file format for %s", filename);
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
    return;
}

#define USERBLOCK_XFER_SIZE     512     /* size of buffer/# of bytes to xfer at a time when copying userblock */

/*-------------------------------------------------------------------------
* local functions
*-------------------------------------------------------------------------
*/
static void  print_dataset_info(hid_t dcpl_id,char *objname,double per, int pr);
static int   do_copy_objects(hid_t fidin,hid_t fidout,trav_table_t *travt,pack_opt_t *options);
static int   copy_user_block(const char *infile, const char *outfile, hsize_t size);
#if defined (H5REPACK_DEBUG_USER_BLOCK)
static void  print_user_block(const char *filename, hid_t fid);
#endif

/*-------------------------------------------------------------------------
* Function: copy_objects
*
* Purpose: duplicate all HDF5 objects in the file
*
* Return: 0, ok, -1 no
*
* Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
*
* Date: October, 23, 2003
*
* Modification:
*   Peter Cao, June 13, 2007
*   Add "-L, --latest" and other options to pack a file with the latest file format
*
*   Peter Cao, September 25, 2007
*   Copy user block when repacking a file
*
*   Pedro Vicente, August 20, 2008
*   Add a user block to file if requested
*
*-------------------------------------------------------------------------
*/



static
int verify_filters(hid_t pid, hid_t tid, int nfilters, filter_info_t *filter)
{
    int           nfilters_dcpl;  /* number of filters in DCPL*/
    unsigned      filt_flags;     /* filter flags */
    H5Z_filter_t  filtn;          /* filter identification number */
    unsigned      cd_values[20];  /* filter client data values */
    size_t        cd_nelmts;      /* filter client number of values */
    char          f_name[256];    /* filter name */
    size_t        size;           /* type size */
    int           i;              /* index */
    unsigned      j;              /* index */

    /* get information about filters */
    if((nfilters_dcpl = H5Pget_nfilters(pid)) < 0)
        return -1;

    /* if we do not have filters and the requested filter is NONE, return 1 */
    if(!nfilters_dcpl &&
       nfilters == 1 &&
       filter[0].filtn == H5Z_FILTER_NONE )
        return 1;

    /* else the numbers of filters must match */
    if (nfilters_dcpl != nfilters )
        return 0;

    /*-------------------------------------------------------------------------
     * build a list with DCPL filters
     *-------------------------------------------------------------------------
     */

    for( i = 0; i < nfilters_dcpl; i++)
    {
        cd_nelmts = NELMTS(cd_values);
        filtn = H5Pget_filter2(pid, (unsigned)i, &filt_flags, &cd_nelmts,
                               cd_values, sizeof(f_name), f_name, NULL);

        /* filter ID */
        if (filtn != filter[i].filtn)
            return 0;

        /* compare client data values. some filters do return local values */
        switch (filtn)
        {

            case H5Z_FILTER_SHUFFLE:

                /* 1 private client value is returned by DCPL */
                if ( cd_nelmts != H5Z_SHUFFLE_TOTAL_NPARMS && filter[i].cd_nelmts != H5Z_SHUFFLE_USER_NPARMS )
                    return 0;

                /* get dataset's type size */
                if((size = H5Tget_size(tid)) <= 0)
                    return -1;

                /* the private client value holds the dataset's type size */
                if ( size != cd_values[0] )
                    return 0;


                break;

            case H5Z_FILTER_SZIP:

                /* 4 private client values are returned by DCPL */
                if ( cd_nelmts != H5Z_SZIP_TOTAL_NPARMS && filter[i].cd_nelmts != H5Z_SZIP_USER_NPARMS )
                    return 0;

                /* "User" parameter for pixels-per-block (index 1) */
                if ( cd_values[H5Z_SZIP_PARM_PPB] != filter[i].cd_values[H5Z_SZIP_PARM_PPB] )
                    return 0;


                break;

            case H5Z_FILTER_NBIT:

                /* only client data values number of values checked */
                if ( H5Z_NBIT_USER_NPARMS != filter[i].cd_nelmts)
                    return 0;



                break;

            case H5Z_FILTER_SCALEOFFSET:

                /* only client data values checked */
                for( j = 0; j < H5Z_SCALEOFFSET_USER_NPARMS; j++)
                {
                    if (cd_values[j] != filter[i].cd_values[j])
                    {
                        return 0;
                    }

                }


                break;

                /* for these filters values must match, no local values set in DCPL */
            case H5Z_FILTER_FLETCHER32:
            case H5Z_FILTER_DEFLATE:

                if ( cd_nelmts != filter[i].cd_nelmts)
                    return 0;

                for( j = 0; j < cd_nelmts; j++)
                {
                    if (cd_values[j] != filter[i].cd_values[j])
                    {
                        return 0;
                    }

                }



                break;



        } /* switch */

    }

    return 1;
}

int copy_objects(const char* fnamein,
                 const char* fnameout,
                 pack_opt_t *options)
{
    hid_t         fidin;
    hid_t         fidout = -1;
    trav_table_t  *travt = NULL;
    hsize_t       ub_size = 0;        /* size of user block */
    hid_t         fcpl = H5P_DEFAULT; /* file creation property list ID */
    hid_t         fapl = H5P_DEFAULT; /* file access property list ID */

    /*-------------------------------------------------------------------------
    * open input file
    *-------------------------------------------------------------------------
    */
    if((fidin = h5tools_fopen(fnamein, H5F_ACC_RDONLY, H5P_DEFAULT, NULL, NULL, (size_t)0)) < 0)
    {
        error_msg("<%s>: %s\n", fnamein, H5FOPENERROR );
        goto out;
    }

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

    /* Check if we need to create a non-default file creation property list */
    if(options->latest || ub_size > 0)
    {
        /* Create file creation property list */
        if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        {
            error_msg("fail to create a file creation property list\n");
            goto out;
        }

        if(ub_size > 0)
        {
            if(H5Pset_userblock(fcpl, ub_size) < 0)
            {
                error_msg("failed to set non-default userblock size\n");
                goto out;
            }
        }

        if(options->latest)
        {
            unsigned i = 0, nindex = 0, mesg_type_flags[5], min_mesg_sizes[5];

            /* Adjust group creation parameters for root group */
            /* (So that it is created in "dense storage" form) */
            if(H5Pset_link_phase_change(fcpl, (unsigned)options->grp_compact, (unsigned)options->grp_indexed) < 0)
            {
                error_msg("fail to adjust group creation parameters for root group\n");
                goto out;
            }

            for(i = 0; i < 5; i++)
            {
                if(options->msg_size[i] > 0)
                {
                    switch(i)
                    {
                        case 0:
                            mesg_type_flags[nindex] = H5O_SHMESG_SDSPACE_FLAG;
                            break;

                        case 1:
                            mesg_type_flags[nindex] = H5O_SHMESG_DTYPE_FLAG;
                            break;

                        case 2:
                            mesg_type_flags[nindex] = H5O_SHMESG_FILL_FLAG;
                            break;

                        case 3:
                            mesg_type_flags[nindex] = H5O_SHMESG_PLINE_FLAG;
                            break;

                        case 4:
                            mesg_type_flags[nindex] = H5O_SHMESG_ATTR_FLAG;
                            break;
                        default:
                            break;
                    } /* end switch */
                    min_mesg_sizes[nindex] = (unsigned)options->msg_size[i];

                    nindex++;
                } /* end if */
            } /* end for */

            if(nindex > 0)
            {
                if(H5Pset_shared_mesg_nindexes(fcpl, nindex) < 0)
                {
                    error_msg("fail to set the number of shared object header message indexes\n");
                    goto out;
                }

                /* msg_size[0]=dataspace, 1=datatype, 2=file value, 3=filter pipleline, 4=attribute */
                for(i = 0; i < (nindex - 1); i++)
                {
                    if(H5Pset_shared_mesg_index(fcpl, i, mesg_type_flags[i], min_mesg_sizes[i]) < 0) {
                        error_msg("fail to configure the specified shared object header message index\n");
                        goto out;
                    } /* end if */
                } /* end for */
            } /* if (nindex>0) */

            /* Create file access property list */
            if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
            {
                error_msg("Could not create file access property list\n");
                goto out;
            } /* end if */

            if(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) < 0)
            {
                error_msg("Could not set property for using latest version of the format\n");
                goto out;
            } /* end if */
        } /* end if */
    } /* end if */




#if defined (H5REPACK_DEBUG_USER_BLOCK)
    print_user_block(fnamein,fidin);
#endif


    /*-------------------------------------------------------------------------
    * set the new user userblock options in the FCPL (before H5Fcreate )
    *-------------------------------------------------------------------------
    */

    if ( options->ublock_size > 0 )
    {
        /* either use the FCPL already created or create a new one */
        if(fcpl != H5P_DEFAULT)
        {
            /* set user block size */
            if(H5Pset_userblock(fcpl, options->ublock_size) < 0)
            {
                error_msg("failed to set userblock size\n");
                goto out;
            }

        }

        else
        {

            /* create a file creation property list */
            if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            {
                error_msg("fail to create a file creation property list\n");
                goto out;
            }

            /* set user block size */
            if(H5Pset_userblock(fcpl, options->ublock_size) < 0)
            {
                error_msg("failed to set userblock size\n");
                goto out;
            }

        }



    }


    /*-------------------------------------------------------------------------
    * set alignment options
    *-------------------------------------------------------------------------
    */


    if (  options->alignment > 0 )
    {
        /* either use the FAPL already created or create a new one */
        if (fapl != H5P_DEFAULT)
        {

            if (H5Pset_alignment(fapl, options->threshold, options->alignment) < 0)
            {
                error_msg("failed to set alignment\n");
                goto out;
            }

        }

        else
        {

            /* create a file access property list */
            if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
            {
                error_msg("Could not create file access property list\n");
                goto out;
            }

            if (H5Pset_alignment(fapl, options->threshold, options->alignment) < 0)
            {
                error_msg("failed to set alignment\n");
                goto out;
            }

        }

    }


    /*-------------------------------------------------------------------------
    * create the output file
    *-------------------------------------------------------------------------
    */


    if(options->verbose)
        printf("Making file <%s>...\n",fnameout);


    if((fidout = H5Fcreate(fnameout,H5F_ACC_TRUNC, fcpl, fapl)) < 0)
    {
        error_msg("<%s>: Could not create file\n", fnameout );
        goto out;
    }


    /*-------------------------------------------------------------------------
    * write a new user block if requested
    *-------------------------------------------------------------------------
    */
    if ( options->ublock_size > 0  )
    {
        if ( copy_user_block( options->ublock_filename, fnameout, options->ublock_size) < 0 )
        {
            error_msg("Could not copy user block. Exiting...\n");
            goto out;

        }
    }

    /*-------------------------------------------------------------------------
    * get list of objects
    *-------------------------------------------------------------------------
    */

    /* init table */
    trav_table_init(&travt);

    /* get the list of objects in the file */
    if(h5trav_gettable(fidin, travt) < 0)
        goto out;

    /*-------------------------------------------------------------------------
    * do the copy
    *-------------------------------------------------------------------------
    */
    if(do_copy_objects(fidin, fidout, travt, options) < 0)
    {
        error_msg("<%s>: Could not copy data to: %s\n", fnamein, fnameout);
        goto out;
    } /* end if */

    /*-------------------------------------------------------------------------
    * do the copy of referenced objects
    * and create hard links
    *-------------------------------------------------------------------------
    */
    if ( do_copy_refobjs(fidin, fidout, travt, options) < 0 )
    {
        printf("h5repack: <%s>: Could not copy data to: %s\n", fnamein, fnameout);
        goto out;
    }

    /*-------------------------------------------------------------------------
    * close
    *-------------------------------------------------------------------------
    */

    if(fapl > 0)
        H5Pclose(fapl);

    if(fcpl > 0)
        H5Pclose(fcpl);

    H5Fclose(fidin);
    H5Fclose(fidout);

    /* free table */
    trav_table_free(travt);
    travt = NULL;

    /*-------------------------------------------------------------------------
    * write only the input file user block if there is no user block file input
    *-------------------------------------------------------------------------
    */

    if( ub_size > 0 && options->ublock_size == 0 )
    {
        if ( copy_user_block(fnamein, fnameout, ub_size) < 0 )
        {
            error_msg("Could not copy user block. Exiting...\n");
            goto out;

        }
    }

    return 0;

    /*-------------------------------------------------------------------------
    * out
    *-------------------------------------------------------------------------
    */

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

/*-------------------------------------------------------------------------
* Function: do_copy_objects
*
* Purpose: duplicate all HDF5 objects in the file
*
* Return: 0, ok, -1 no
*
* Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
*
* Date: October, 23, 2003
*
* Modifications:
*
*  July 2004:     Introduced the extra EC or NN option for SZIP
*
*  December 2004: Added a check for H5Dcreate; if the dataset cannot be created
*                  with the requested filter, use the input one
*
*  October 2006:  Read/write using the file type by default.
*
*  October 2006:  Read by hyperslabs for big datasets.
*
*  A threshold of H5TOOLS_MALLOCSIZE (128 MB) is the limit upon which I/O hyperslab is done
*  i.e., if the memory needed to read a dataset is greater than this limit,
*  then hyperslab I/O is done instead of one operation I/O
*  For each dataset, the memory needed is calculated according to
*
*  memory needed = number of elements * size of each element
*
*  if the memory needed is lower than H5TOOLS_MALLOCSIZE, then the following operations
*  are done
*
*  H5Dread( input_dataset1 )
*  H5Dread( input_dataset2 )
*
*  with all elements in the datasets selected. If the memory needed is greater than
*  H5TOOLS_MALLOCSIZE, then the following operations are done instead:
*
*  a strip mine is defined for each dimension k (a strip mine is defined as a
*  hyperslab whose size is memory manageable) according to the formula
*
*  (1) strip_mine_size[k ] = MIN(dimension[k ], H5TOOLS_BUFSIZE / size of memory type)
*
*  where H5TOOLS_BUFSIZE is a constant currently defined as 1MB. This formula assures
*  that for small datasets (small relative to the H5TOOLS_BUFSIZE constant), the strip
*  mine size k is simply defined as its dimension k, but for larger datasets the
*  hyperslab size is still memory manageable.
*  a cycle is done until the number of elements in the dataset is reached. In each
*  iteration, two parameters are defined for the function H5Sselect_hyperslab,
*  the start and size of each hyperslab, according to
*
*  (2) hyperslab_size [k] = MIN(dimension[k] - hyperslab_offset[k], strip_mine_size [k])
*
*  where hyperslab_offset [k] is initially set to zero, and later incremented in
*  hyperslab_size[k] offsets. The reason for the operation
*
*  dimension[k] - hyperslab_offset[k]
*
*  in (2) is that, when using the strip mine size, it assures that the "remaining" part
*  of the dataset that does not fill an entire strip mine is processed.
*
*  November 2006:  Use H5Ocopy in the copy of objects. The logic for using
*   H5Ocopy or not is if a change of filters or layout is requested by the user
*   then use read/write else use H5Ocopy.
*
* May, 1, 2008: Add a printing of the compression ratio of old size / new size
*
*-------------------------------------------------------------------------
*/





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

    if (options->verbose)
    {
        printf("-----------------------------------------\n");
        printf(" Type     Filter (Compression)     Name\n");
        printf("-----------------------------------------\n");
    }

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

                if (options->verbose)
                {
                    printf(FORMAT_OBJ,"group",travt->objs[i].name );
                }

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

                /*-------------------------------------------------------------------------
                * copy attrs
                *-------------------------------------------------------------------------
                */
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

                    /*-------------------------------------------------------------------------
                    * check if the dataset creation property list has filters that
                    * are not registered in the current configuration
                    * 1) the external filters GZIP and SZIP might not be available
                    * 2) the internal filters might be turned off
                    *-------------------------------------------------------------------------
                    */
                    if (h5tools_canreadf((travt->objs[i].name),dcpl_id)==1)
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

                            /*-------------------------------------------------------------------------
                            * create the output dataset;
                            * disable error checking in case the dataset cannot be created with the
                            * modified dcpl; in that case use the original instead
                            *-------------------------------------------------------------------------
                            */
                            H5E_BEGIN_TRY {
                                    dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_out, H5P_DEFAULT);
                                } H5E_END_TRY;

                            if(dset_out == FAIL)
                            {
                                if(options->verbose)
                                    printf(" warning: could not create dataset <%s>. Applying original settings\n",
                                           travt->objs[i].name);

                                if((dset_out = H5Dcreate2(fidout, travt->objs[i].name, wtype_id, f_space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0)
                                    goto error;
                                apply_f = 0;
                            }

                            /*-------------------------------------------------------------------------
                            * read/write
                            *-------------------------------------------------------------------------
                            */
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
                                        hsize_t size = H5TOOLS_BUFSIZE / sm_nbytes;
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
                                                goto error;
                                            if (H5Sselect_hyperslab(sm_space, H5S_SELECT_SET, zero, NULL, &hs_nelmts, NULL) < 0)
                                                goto error;
                                        }
                                        else
                                        {
                                            H5Sselect_all(f_space_id);
                                            H5Sselect_all(sm_space);
                                            hs_nelmts = 1;
                                        } /* rank */

                                        /* read/write */
                                        if (H5Dread(dset_in, wtype_id, sm_space, f_space_id, H5P_DEFAULT, sm_buf) < 0)
                                            goto error;
                                        if (H5Dwrite(dset_out, wtype_id, sm_space, f_space_id, H5P_DEFAULT, sm_buf) < 0)
                                            goto error;

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
                                        free(sm_buf);
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

                                    print_dataset_info(dcpl_out,travt->objs[i].name,ratio,1);
                                }
                                else
                                    print_dataset_info(dcpl_id,travt->objs[i].name,ratio,0);

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
                                goto error;

                            /*close */
                            if (H5Dclose(dset_out) < 0)
                                goto error;

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
                    /*-------------------------------------------------------------------------
                    * we do not have request for filter/chunking use H5Ocopy instead
                    *-------------------------------------------------------------------------
                    */
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


                    if (options->verbose)
                        printf(FORMAT_OBJ,"dset",travt->objs[i].name );


                } /* end do we have request for filter/chunking */


                break;

                /*-------------------------------------------------------------------------
                * H5TRAV_TYPE_NAMED_DATATYPE
                *-------------------------------------------------------------------------
                */
            case H5TRAV_TYPE_NAMED_DATATYPE:

                if(options->verbose)
                    printf(FORMAT_OBJ, "type", travt->objs[i].name);

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

                if(options->verbose)
                    printf(FORMAT_OBJ, "link", travt->objs[i].name);

                if(H5Lcopy(fidin, travt->objs[i].name,fidout, travt->objs[i].name, H5P_DEFAULT, H5P_DEFAULT) < 0)
                    goto error;

                if (options->verbose)
                    printf(FORMAT_OBJ,"link",travt->objs[i].name );

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

int
get_option(int argc, const char **argv, const char *opts, const struct long_options *l_opts)
{
    static int sp = 1;    /* character index in current token */
    int opt_opt = '?';    /* option character passed back to user */

    if (sp == 1) {
        /* check for more flag-like tokens */
        if (opt_ind >= argc || argv[opt_ind][0] != '-' || argv[opt_ind][1] == '\0') {
            return EOF;
        } else if (strcmp(argv[opt_ind], "--") == 0) {
            opt_ind++;
            return EOF;
        }
    }

    if (sp == 1 && argv[opt_ind][0] == '-' && argv[opt_ind][1] == '-') {
        /* long command line option */
        const char *arg = &argv[opt_ind][2];
        int i;

        for (i = 0; l_opts && l_opts[i].name; i++) {
            size_t len = strlen(l_opts[i].name);

            if (strncmp(arg, l_opts[i].name, len) == 0) {
                /* we've found a matching long command line flag */
                opt_opt = l_opts[i].shortval;

                if (l_opts[i].has_arg != no_arg) {
                    if (arg[len] == '=') {
                        opt_arg = &arg[len + 1];
                    } else if (opt_ind < (argc - 1) && argv[opt_ind + 1][0] != '-') {
                        opt_arg = argv[++opt_ind];
                    } else if (l_opts[i].has_arg == require_arg) {
                        if (1)
                            fprintf(stderr,
                                      "%s: option required for \"--%s\" flag\n",
                                      argv[0], arg);

                        opt_opt = '?';
                    }
                } else {
                    if (arg[len] == '=') {
                        if (1)
                            fprintf(stderr,
                                      "%s: no option required for \"%s\" flag\n",
                                      argv[0], arg);

                        opt_opt = '?';
                    }

                    opt_arg = NULL;
                }

                break;
            }
        }

        if (l_opts[i].name == NULL) {
            /* exhausted all of the l_opts we have and still didn't match */
            if (1)
                fprintf(stderr, "%s: unknown option \"%s\"\n", argv[0], arg);

            opt_opt = '?';
        }

        opt_ind++;
        sp = 1;
    } else {
        register char *cp;    /* pointer into current token */

        /* short command line option */
        opt_opt = argv[opt_ind][sp];

        if (opt_opt == ':' || (cp = (char*)strchr(opts, opt_opt)) == 0) {
            if (1)
                fprintf(stderr, "%s: unknown option \"%c\"\n",
                          argv[0], opt_opt);

            /* if no chars left in this token, move to next token */
            if (argv[opt_ind][++sp] == '\0') {
                opt_ind++;
                sp = 1;
            }

            return '?';
        }

        if (*++cp == ':') {
            /* if a value is expected, get it */
            if (argv[opt_ind][sp + 1] != '\0') {
                /* flag value is rest of current token */
                opt_arg = &argv[opt_ind++][sp + 1];
            } else if (++opt_ind >= argc) {
                if (1)
                    fprintf(stderr,
                              "%s: value expected for option \"%c\"\n",
                              argv[0], opt_opt);

                opt_opt = '?';
            } else {
                /* flag value is next token */
                opt_arg = argv[opt_ind++];
            }

            sp = 1;
        }

            /* wildcard argument */
        else if (*cp == '*')
        {
            /* check the next argument */
            opt_ind++;
            /* we do have an extra argument, check if not last */
            if ( argv[opt_ind][0] != '-' && (opt_ind+1) < argc )
            {
                opt_arg = argv[opt_ind++];
            }
            else
            {
                opt_arg = NULL;
            }
        }

        else
        {
            /* set up to look at next char in token, next time */
            if (argv[opt_ind][++sp] == '\0') {
                /* no more in current token, so setup next token */
                opt_ind++;
                sp = 1;


            }

            opt_arg = NULL;
        }
    }

    /* return the current flag character found */
    return opt_opt;
}

static void print_dataset_info(hid_t dcpl_id,
                               char *objname,
                               double ratio,
                               int pr)
{
    char         strfilter[255];
#if defined (PRINT_DEBUG )
    char         temp[255];
#endif
    int          nfilters;       /* number of filters */
    unsigned     filt_flags;     /* filter flags */
    H5Z_filter_t filtn;          /* filter identification number */
    unsigned     cd_values[20];  /* filter client data values */
    size_t       cd_nelmts;      /* filter client number of values */
    char         f_objname[256];    /* filter objname */
    int          i;


    strcpy(strfilter,"\0");

    /* get information about input filters */
    if((nfilters = H5Pget_nfilters(dcpl_id)) < 0)
        return;

    for(i = 0; i < nfilters; i++) {
        cd_nelmts = NELMTS(cd_values);

        filtn = H5Pget_filter2(dcpl_id, (unsigned)i, &filt_flags, &cd_nelmts,
                               cd_values, sizeof(f_objname), f_objname, NULL);

        switch(filtn) {
            default:
                break;

            case H5Z_FILTER_DEFLATE:
                strcat(strfilter,"GZIP ");

#if defined (PRINT_DEBUG)
            {
                unsigned level=cd_values[0];
                sprintf(temp,"(%d)",level);
                strcat(strfilter,temp);
            }
#endif
                break;

            case H5Z_FILTER_SZIP:
                strcat(strfilter,"SZIP ");

#if defined (PRINT_DEBUG)
            {
                unsigned options_mask=cd_values[0]; /* from dcpl, not filt*/
                unsigned ppb=cd_values[1];
                sprintf(temp,"(%d,",ppb);
                strcat(strfilter,temp);
                if (options_mask & H5_SZIP_EC_OPTION_MASK)
                    strcpy(temp,"EC) ");
                else if (options_mask & H5_SZIP_NN_OPTION_MASK)
                    strcpy(temp,"NN) ");
            }
            strcat(strfilter,temp);

#endif

                break;

            case H5Z_FILTER_SHUFFLE:
                strcat(strfilter,"SHUF ");
                break;

            case H5Z_FILTER_FLETCHER32:
                strcat(strfilter,"FLET ");
                break;

            case H5Z_FILTER_NBIT:
                strcat(strfilter,"NBIT ");
                break;

            case H5Z_FILTER_SCALEOFFSET:
                strcat(strfilter,"SCALEOFFSET ");
                break;
        } /* switch */
    }/*i*/

    if(!pr)
        printf(FORMAT_OBJ,"dset",objname );
    else
    {
        char str[255], temp[20];
        strcpy(str,"dset     ");
        strcat(str,strfilter);
        sprintf(temp,"  (%.3f:1)",ratio);
        strcat(str,temp);
        printf(FORMAT_OBJ,str,objname);
    }
}


static int
copy_user_block(const char *infile, const char *outfile, hsize_t size)
{
    int infid = -1, outfid = -1;        /* File descriptors */
    int status = 0;                     /* Return value */

    /* User block must be any power of 2 equal to 512 or greater (512, 1024, 2048, etc.) */
    assert(size > 0);

    std::fstream fs;

    /* Open files */
    if((infid = HDopen(infile, O_RDONLY, 0)) < 0) {
        status = -1;
        goto done;
    }
    if((outfid = HDopen(outfile, O_WRONLY, 0644)) < 0) {
        status = -1;
        goto done;
    }

    /* Copy the userblock from the input file to the output file */
    while(size > 0) {
        ssize_t nread, nbytes;                  /* # of bytes transfered, etc. */
        char rbuf[USERBLOCK_XFER_SIZE];         /* Buffer for reading */
        const char *wbuf;                       /* Pointer into buffer, for writing */

        /* Read buffer from source file */
        if(size > USERBLOCK_XFER_SIZE)
            nread = read(infid, rbuf, (size_t)USERBLOCK_XFER_SIZE);
        else
            nread = read(infid, rbuf, (size_t)size);
        if(nread < 0) {
            status = -1;
            goto done;
        } /* end if */

        /* Write buffer to destination file */
        /* (compensating for interrupted writes & checking for errors, etc.) */
        nbytes = nread;
        wbuf = rbuf;
        while(nbytes > 0) {
            ssize_t nwritten;        /* # of bytes written */

            do {
                nwritten = write(outfid, wbuf, (size_t)nbytes);
            } while(-1 == nwritten && EINTR == errno);
            if(-1 == nwritten) { /* error */
                status = -1;
                goto done;
            } /* end if */
            assert(nwritten > 0);
            assert(nwritten <= nbytes);

            /* Update # of bytes left & offset in buffer */
            nbytes -= nwritten;
            wbuf += nwritten;
            assert(nbytes == 0 || wbuf < (rbuf + USERBLOCK_XFER_SIZE));
        } /* end while */

        /* Update size of userblock left to transfer */
        size -= nread;
    } /* end while */

    done:
    if(infid > 0)
        close(infid);
    if(outfid > 0)
        close(outfid);

    return status;
}


static void print_user_block(const char *filename, hid_t fid)
{
    int     fh;      /* file handle  */
    hsize_t ub_size; /* user block size */
    hsize_t size;    /* size read */
    hid_t   fcpl;    /* file creation property list ID for HDF5 file */
    int     i;

    /* get user block size */
    if(( fcpl = H5Fget_create_plist(fid)) < 0)
    {
        error_msg("failed to retrieve file creation property list\n");
        goto done;
    }

    if(H5Pget_userblock(fcpl, &ub_size) < 0)
    {
        error_msg("failed to retrieve userblock size\n");
        goto done;
    }

    if(H5Pclose(fcpl) < 0)
    {
        error_msg("failed to close property list\n");
        goto done;
    }

    /* open file */
    if((fh = HDopen(filename, O_RDONLY, 0)) < 0)
    {
        goto done;
    }

    size = ub_size;

    /* read file */
    while(size > 0)
    {
        ssize_t nread;                  /* # of bytes read */
        char rbuf[USERBLOCK_XFER_SIZE]; /* buffer for reading */

        /* read buffer */
        if(size > USERBLOCK_XFER_SIZE)
            nread = read(fh, rbuf, (size_t)USERBLOCK_XFER_SIZE);
        else
            nread = read(fh, rbuf, (size_t)size);

        for(i = 0; i < nread; i++)
        {

            printf("%c ", rbuf[i]);

        }
        printf("\n");

        if(nread < 0)
        {
            goto done;
        }


        /* update size of userblock left to transfer */
        size -= nread;
    }

done:
    if(fh > 0)
        close(fh);


    return;
}

int verify_layout(hid_t pid,
                  pack_info_t *obj)
{
    hsize_t      chsize[64];     /* chunk size in elements */
    H5D_layout_t layout;         /* layout */
    int          nfilters;       /* number of filters */
    int          rank;           /* rank */
    int          i;              /* index */

    /* check if we have filters in the input object */
    if ((nfilters = H5Pget_nfilters(pid)) < 0)
        return -1;

    /* a non chunked layout was requested on a filtered object */
    if (nfilters && obj->layout!=H5D_CHUNKED)
        return 0;

    /* get layout */
    if ((layout = H5Pget_layout(pid)) < 0)
        return -1;

    if (obj->layout != layout)
        return 0;

    if (layout==H5D_CHUNKED)
    {
        if ((rank = H5Pget_chunk(pid,NELMTS(chsize),chsize/*out*/)) < 0)
            return -1;
        if (obj->chunk.rank != rank)
            return 0;
        for ( i=0; i<rank; i++)
            if (chsize[i] != obj->chunk.chunk_lengths[i])
                return 0;
    }

    return 1;
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



pack_info_t* options_get_object( const char *path,
                                 pack_opttbl_t *table )
{
    unsigned int i;

    for ( i = 0; i < table->nelems; i++)
    {
        /* found it */
        if (strcmp(table->objs[i].path,path)==0)
        {
            return (&table->objs[i]);
        }
    }

    return NULL;
}


int cucorepack(const char* infile,
             const char* outfile,
             pack_opt_t *options)
{
    /* check input */
    if (check_options(options)<0)
        return -1;

    /* check for objects in input that are in the file */
    if (check_objects(infile,options) < 0)
        return -1;

    /* copy the objects  */
    if (copy_objects(infile,outfile,options) < 0)
        return -1;


    return 0;
}

int h5repack_verify(const char *fname,
                    pack_opt_t *options)
{
    hid_t        fid;  /* file ID */
    hid_t        did;  /* dataset ID */
    hid_t        pid;  /* dataset creation property list ID */
    hid_t        sid;  /* space ID */
    hid_t        tid;  /* type ID */
    unsigned int i;
    trav_table_t *travt = NULL;
    int          ok = 1;

    /* open the file */
    if((fid = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0 )
        return -1;

    for(i = 0; i < options->op_tbl->nelems; i++)
    {
        char* name = options->op_tbl->objs[i].path;
        pack_info_t *obj = &options->op_tbl->objs[i];

        /*-------------------------------------------------------------------------
         * open
         *-------------------------------------------------------------------------
         */
        if((did = H5Dopen2(fid, name, H5P_DEFAULT)) < 0)
            goto error;
        if((sid = H5Dget_space(did)) < 0)
            goto error;
        if((pid = H5Dget_create_plist(did)) < 0)
            goto error;
        if((tid = H5Dget_type(did)) < 0)
            goto error;

        /*-------------------------------------------------------------------------
         * filter check
         *-------------------------------------------------------------------------
         */
        if(verify_filters(pid, tid, obj->nfilters, obj->filter) <= 0)
            ok = 0;


        /*-------------------------------------------------------------------------
         * layout check
         *-------------------------------------------------------------------------
         */
        if((obj->layout != -1) && (verify_layout(pid, obj) == 0))
            ok = 0;

        /*-------------------------------------------------------------------------
         * close
         *-------------------------------------------------------------------------
         */
        if(H5Pclose(pid) < 0)
            goto error;
        if (H5Sclose(sid) < 0)
            goto error;
        if (H5Dclose(did) < 0)
            goto error;
        if (H5Tclose(tid) < 0)
            goto error;

    }


    /*-------------------------------------------------------------------------
     * check for the "all" objects option
     *-------------------------------------------------------------------------
     */

    if(options->all_filter == 1 || options->all_layout == 1)
    {

        /* init table */
        trav_table_init(&travt);

        /* get the list of objects in the file */
        if(h5trav_gettable(fid, travt) < 0)
            goto error;

        for(i = 0; i < travt->nobjs; i++)
        {
            char *name = travt->objs[i].name;

            if(travt->objs[i].type == H5TRAV_TYPE_DATASET)
            {

                /*-------------------------------------------------------------------------
                 * open
                 *-------------------------------------------------------------------------
                 */
                if((did = H5Dopen2(fid, name, H5P_DEFAULT)) < 0)
                    goto error;
                if((sid = H5Dget_space(did)) < 0)
                    goto error;
                if((pid = H5Dget_create_plist(did)) < 0)
                    goto error;
                if((tid = H5Dget_type(did)) < 0)
                    goto error;

                /*-------------------------------------------------------------------------
                 * filter check
                 *-------------------------------------------------------------------------
                 */
                if(options->all_filter == 1)
                {

                    if(verify_filters(pid, tid, options->n_filter_g, options->filter_g) <= 0)
                        ok = 0;
                }

                /*-------------------------------------------------------------------------
                 * layout check
                 *-------------------------------------------------------------------------
                 */
                if(options->all_layout == 1)
                {
                    pack_info_t pack;
                    init_packobject(&pack);
                    pack.layout = options->layout_g;
                    pack.chunk = options->chunk_g;
                    if(verify_layout(pid, &pack) == 0)
                        ok = 0;
                }


                /*-------------------------------------------------------------------------
                 * close
                 *-------------------------------------------------------------------------
                 */
                if (H5Pclose(pid) < 0)
                    goto error;
                if (H5Sclose(sid) < 0)
                    goto error;
                if (H5Dclose(did) < 0)
                    goto error;
                if (H5Tclose(tid) < 0)
                    goto error;
            } /* if */

        } /* i */

        /* free table */
        trav_table_free(travt);
    }

    /*-------------------------------------------------------------------------
     * close
     *-------------------------------------------------------------------------
     */

    if (H5Fclose(fid) < 0)
        return -1;

    return ok;

    error:
    H5E_BEGIN_TRY {
            H5Pclose(pid);
            H5Sclose(sid);
            H5Dclose(did);
            H5Fclose(fid);
            if (travt)
                trav_table_free(travt);
        } H5E_END_TRY;
    return -1;
} /* h5repack_verify() */




int h5repack_cmp_pl(const char *fname1,
                    const char *fname2)
{
    hid_t         fid1=-1;         /* file ID */
    hid_t         fid2=-1;         /* file ID */
    hid_t         dset1=-1;        /* dataset ID */
    hid_t         dset2=-1;        /* dataset ID */
    hid_t         gid=-1;          /* group ID */
    hid_t         dcpl1=-1;        /* dataset creation property list ID */
    hid_t         dcpl2=-1;        /* dataset creation property list ID */
    hid_t         gcplid=-1;       /* group creation property list */
    unsigned      crt_order_flag1; /* group creation order flag */
    unsigned      crt_order_flag2; /* group creation order flag */
    trav_table_t  *trav=NULL;
    int           ret=1;
    unsigned int  i;

    /*-------------------------------------------------------------------------
     * open the files
     *-------------------------------------------------------------------------
     */

    /* disable error reporting */
    H5E_BEGIN_TRY
        {

            /* Open the files */
            if ((fid1=H5Fopen(fname1,H5F_ACC_RDONLY,H5P_DEFAULT)) < 0 )
            {
                error_msg("<%s>: %s\n", fname1, H5FOPENERROR );
                return -1;
            }
            if ((fid2=H5Fopen(fname2,H5F_ACC_RDONLY,H5P_DEFAULT)) < 0 )
            {
                error_msg("<%s>: %s\n", fname2, H5FOPENERROR );
                H5Fclose(fid1);
                return -1;
            }
            /* enable error reporting */
        } H5E_END_TRY;

    /*-------------------------------------------------------------------------
     * get file table list of objects
     *-------------------------------------------------------------------------
     */
    trav_table_init(&trav);
    if(h5trav_gettable(fid1, trav) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * traverse the suppplied object list
     *-------------------------------------------------------------------------
     */
    for(i = 0; i < trav->nobjs; i++)
    {

        if(trav->objs[i].type == H5TRAV_TYPE_GROUP)
        {

            if ((gid = H5Gopen2(fid1, trav->objs[i].name, H5P_DEFAULT)) < 0)
                goto error;
            if ((gcplid = H5Gget_create_plist(gid)) < 0)
                goto error;
            if (H5Pget_link_creation_order(gcplid, &crt_order_flag1) < 0)
                goto error;
            if (H5Pclose(gcplid) < 0)
                goto error;
            if (H5Gclose(gid) < 0)
                goto error;

            if ((gid = H5Gopen2(fid2, trav->objs[i].name, H5P_DEFAULT)) < 0)
                goto error;
            if ((gcplid = H5Gget_create_plist(gid)) < 0)
                goto error;
            if (H5Pget_link_creation_order(gcplid, &crt_order_flag2) < 0)
                goto error;
            if (H5Pclose(gcplid) < 0)
                goto error;
            if (H5Gclose(gid) < 0)
                goto error;

            if ( crt_order_flag1 != crt_order_flag2 )
            {
                error_msg("property lists for <%s> are different\n",trav->objs[i].name);
                goto error;
            }

        }



        else if(trav->objs[i].type == H5TRAV_TYPE_DATASET)
        {
            if((dset1 = H5Dopen2(fid1, trav->objs[i].name, H5P_DEFAULT)) < 0)
                goto error;
            if((dset2 = H5Dopen2(fid2, trav->objs[i].name, H5P_DEFAULT)) < 0)
                goto error;
            if((dcpl1 = H5Dget_create_plist(dset1)) < 0)
                goto error;
            if((dcpl2 = H5Dget_create_plist(dset2)) < 0)
                goto error;

            /*-------------------------------------------------------------------------
             * compare the property lists
             *-------------------------------------------------------------------------
             */
            if((ret = H5Pequal(dcpl1, dcpl2)) < 0)
                goto error;

            if(ret == 0)
            {
                error_msg("property lists for <%s> are different\n",trav->objs[i].name);
                goto error;
            }

            /*-------------------------------------------------------------------------
             * close
             *-------------------------------------------------------------------------
             */
            if(H5Pclose(dcpl1) < 0)
                goto error;
            if(H5Pclose(dcpl2) < 0)
                goto error;
            if(H5Dclose(dset1) < 0)
                goto error;
            if(H5Dclose(dset2) < 0)
                goto error;
        } /*if*/
    } /*i*/

    /*-------------------------------------------------------------------------
     * free
     *-------------------------------------------------------------------------
     */

    trav_table_free(trav);

    /*-------------------------------------------------------------------------
     * close
     *-------------------------------------------------------------------------
     */

    H5Fclose(fid1);
    H5Fclose(fid2);
    return ret;

    /*-------------------------------------------------------------------------
     * error
     *-------------------------------------------------------------------------
     */

    error:
    H5E_BEGIN_TRY
        {
            H5Pclose(dcpl1);
            H5Pclose(dcpl2);
            H5Dclose(dset1);
            H5Dclose(dset2);
            H5Fclose(fid1);
            H5Fclose(fid2);
            H5Pclose(gcplid);
            H5Gclose(gid);
            trav_table_free(trav);
        } H5E_END_TRY;
    return -1;

}


void h5repack_init (pack_opt_t *options,
                   int verbose)
{
    int k, n;
    memset(options,0,sizeof(pack_opt_t));
    options->min_comp = 1024;
    options->verbose  = verbose;

    for ( n = 0; n < H5_REPACK_MAX_NFILTERS; n++)
    {
        options->filter_g[n].filtn  = -1;
        options->filter_g[n].cd_nelmts  = 0;
        for ( k = 0; k < CD_VALUES; k++)
            options->filter_g[n].cd_values[k] = 0;
    }

    options_table_init(&(options->op_tbl));
}

int h5repack::noMain(int argc, const char **argv)
{

    pack_opt_t    options;            /*the global options */
    int           ret=-1;

    /* initialize options  */
    h5repack_init(&options,0);

    get_option(argc, argv, s_opts, l_opts);

    /* parse the -f filter option */
    if (h5repack_addfilter( "GZIP=9", &options)<0)
    {
        error_msg("in parsing filter\n");
        exit(EXIT_FAILURE);
    }

    /* get file names if they were not yet got */
    if ( has_i_o == 0 )
    {

        if ( argv[ opt_ind ] != NULL && argv[ opt_ind + 1 ] != NULL )
        {
            infile = argv[ opt_ind ];
            outfile = argv[ opt_ind + 1 ];

            if ( strcmp( infile, outfile ) == 0 )
            {
                error_msg("file names cannot be the same\n");
                exit(EXIT_FAILURE);

            }
        }

        else
        {
            error_msg("file names missing\n");
            exit(EXIT_FAILURE);
        }
    }


    /* pack it */
    ret=cucorepack(infile,outfile,&options);

    /* free tables */
    h5repack_end(&options);

    if (ret==-1)
        return 1;
    else
        return 0;
}

static void usage(const char *prog)
{
    printf("Cuco\n", prog);

}



