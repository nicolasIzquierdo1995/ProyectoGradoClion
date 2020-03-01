#include "../headers/h5trav.hpp"
#include "../headers/hdfalloc.hpp"
#include "../headers/utils.hpp"
#include <assert.h>
using namespace std;
using namespace H5;
using namespace hdfalloc;
using namespace h5trav;
/*-------------------------------------------------------------------------
 * local typedefs
 *-------------------------------------------------------------------------
 */
typedef struct trav_addr_t_obj {
    haddr_t addr;
    char *path;
} trav_addr_t_obj;

typedef struct trav_addr_t {
    size_t      nalloc;
    size_t      nused;
    trav_addr_t_obj *objs;
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

static void trav_table_add(trav_table_t *table,
                           const char *path,
                           const H5O_info_t *oinfo)
{
    size_t newone;

    if(table->nobjs == table->size) {
        table->size = MAX(1, table->size * 2);
        table->objs = (trav_obj_t*)HDrealloc(table->objs, table->size * sizeof(trav_obj_t));
    } /* end if */

    newone = table->nobjs++;
    table->objs[newone].objno = oinfo ? oinfo->addr : HADDR_UNDEF;
    table->objs[newone].flags[0] = table->objs[newone].flags[1] = 0;
    table->objs[newone].name = (char *)HDstrdup(path);
    table->objs[newone].type = oinfo ? (h5trav_type_t)oinfo->type : H5TRAV_TYPE_LINK;
    table->objs[newone].nlinks = 0;
    table->objs[newone].sizelinks = 0;
    table->objs[newone].links = NULL;
}

static void trav_table_addlink(trav_table_t *table, haddr_t objno, const char *path)
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
                table->objs[i].links = (trav_link_t*)HDrealloc(table->objs[i].links, table->objs[i].sizelinks * sizeof(trav_link_t));
            } /* end if */

            /* insert it */
            n = table->objs[i].nlinks++;
            table->objs[i].links[n].new_name = (char *)HDstrdup(path);

            return;
        } /* end for */
    } /* end for */

    assert(0 && "object not in table?!?");
}

static void trav_addr_add(trav_addr_t *visited, haddr_t addr, const char *path)
{
    size_t idx;         /* Index of address to use */

    /* Allocate space if necessary */
    if(visited->nused == visited->nalloc) {
        visited->nalloc = MAX(1, visited->nalloc * 2);;
        visited->objs = (trav_addr_t_obj*)HDrealloc(visited->objs, visited->nalloc * sizeof(visited->objs[0]));
    } /* end if */

    /* Append it */
    idx = visited->nused++;
    visited->objs[idx].addr = addr;
    visited->objs[idx].path = HDstrdup(path);
} /* end trav_addr_add() */

static const char* trav_addr_visited(trav_addr_t *visited, haddr_t addr)
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

static herr_t traverse_cb(hid_t loc_id, const char *path, const H5L_info_t *linfo,
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

        if(NULL == (new_name = (char*)HDmalloc(base_len + add_slash + strlen(path) + 1)))
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

static int traverse(hid_t file_id, const char *grp_name, hbool_t visit_start,
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

static int trav_table_visit_obj(const char *path, const H5O_info_t *oinfo,
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

static int trav_table_visit_lnk(const char *path, const H5L_info_t *linfo, void *udata)
{
    /* Add the link to the 'table' struct */
    trav_table_add((trav_table_t *) udata, path, NULL);

    return(0);
} /* end trav_table_visit_lnk() */

int h5trav::h5trav_gettable(hid_t fid, trav_table_t *table)
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

void h5trav::trav_table_init(trav_table_t **tbl)
{
    trav_table_t* table = (trav_table_t*) HDmalloc(sizeof(trav_table_t));

    table->size = 0;
    table->nobjs = 0;
    table->objs = NULL;

    *tbl = table;
}

void h5trav::trav_table_free( trav_table_t *table )
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