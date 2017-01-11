/*
 * Copyright (c) 2013-2015, Lawrence Livermore National Security, LLC.
 *   Produced at the Lawrence Livermore National Laboratory
 *   CODE-673838
 *
 * Copyright (c) 2006-2007,2011-2015, Los Alamos National Security, LLC.
 *   (LA-CC-06-077, LA-CC-10-066, LA-CC-14-046)
 *
 * Copyright (2013-2015) UT-Battelle, LLC under Contract No.
 *   DE-AC05-00OR22725 with the Department of Energy.
 *
 * Copyright (c) 2015, DataDirect Networks, Inc.
 * 
 * All rights reserved.
 *
 * This file is part of mpiFileUtils.
 * For details, see https://github.com/hpc/fileutils.
 * Please also read the LICENSE file.
*/

#include "mfu.h"
#include "mpi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* initialize fields in param */
static void mfu_param_path_init(mfu_param_path* param)
{
    /* initialize all fields */
    if (param != NULL) {
        param->orig = NULL;
        param->path = NULL;
        param->path_stat_valid = 0;
        param->target = NULL;
        param->target_stat_valid = 0;
    }

    return;
}

/* pack all fields as 64-bit values, except for times which we
 * pack as two 64-bit values */
static size_t mfu_pack_stat_size(void)
{
    size_t size = 16 * 8;
    return size;
}

static void mfu_pack_stat(char** pptr, const struct stat* s)
{
    mfu_pack_uint64(pptr, (uint64_t) s->st_dev);
    mfu_pack_uint64(pptr, (uint64_t) s->st_ino);
    mfu_pack_uint64(pptr, (uint64_t) s->st_mode);
    mfu_pack_uint64(pptr, (uint64_t) s->st_nlink);
    mfu_pack_uint64(pptr, (uint64_t) s->st_uid);
    mfu_pack_uint64(pptr, (uint64_t) s->st_gid);
    mfu_pack_uint64(pptr, (uint64_t) s->st_rdev);
    mfu_pack_uint64(pptr, (uint64_t) s->st_size);
    mfu_pack_uint64(pptr, (uint64_t) s->st_blksize);
    mfu_pack_uint64(pptr, (uint64_t) s->st_blocks);
    mfu_pack_uint64(pptr, (uint64_t) s->st_atime);
    mfu_pack_uint64(pptr, (uint64_t) 0);
    mfu_pack_uint64(pptr, (uint64_t) s->st_mtime);
    mfu_pack_uint64(pptr, (uint64_t) 0);
    mfu_pack_uint64(pptr, (uint64_t) s->st_ctime);
    mfu_pack_uint64(pptr, (uint64_t) 0);

    return;
}

static void mfu_unpack_stat(const char** pptr, struct stat* s)
{
    uint64_t val;

    mfu_unpack_uint64(pptr, &val);
    s->st_dev = (dev_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_ino = (ino_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_mode = (mode_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_nlink = (nlink_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_uid = (uid_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_gid = (gid_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_rdev = (dev_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_size = (off_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_blksize = (blksize_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_blocks = (blkcnt_t) val;

    mfu_unpack_uint64(pptr, &val);
    s->st_atime = (time_t) val;

    mfu_unpack_uint64(pptr, &val);
    /* atime nsecs */

    mfu_unpack_uint64(pptr, &val);
    s->st_mtime = (time_t) val;

    mfu_unpack_uint64(pptr, &val);
    /* mtime nsecs */

    mfu_unpack_uint64(pptr, &val);
    s->st_ctime = (time_t) val;

    mfu_unpack_uint64(pptr, &val);
    /* ctime nsecs */

    return;
}

/* given a pointer to a string, which may be the NULL pointer,
 * return number of bytes needed to pack the string */
static size_t mfu_pack_str_size(const char* str)
{
    /* uint32_t flag to indicate whether string is non-NULL */
    size_t bytes = 4;

    /* if we have a string, add in its length */
    if (str != NULL) {
        bytes += strlen(str) + 1;
    }

    return bytes;
}

/* given a pointer to a string, which may be the NULL pointer,
 * pack the string into the specified buffer and update the
 * pointer to point to next free byte */
static void mfu_pack_str(char** pptr, const char* str)
{
    /* check whether we have a string */
    if (str != NULL) {
        /* set flag to indicate that we have string */
        mfu_pack_uint32(pptr, 1);

        /* get pointer to start of buffer */
        char* ptr = *pptr;

        /* copy in string */
        size_t len = strlen(str) + 1;
        strncpy(ptr, str, len);

        /* update caller's pointer */
        *pptr += len;
    } else {
        /* set flag to indicate that we don't have str */
        mfu_pack_uint32(pptr, 0);
    }
}

/* unpack a string from a buffer, and return as newly allocated memory,
 * caller must free returned string */
static void mfu_unpack_str(const char** pptr, char** pstr)
{
    /* assume we don't have a string */
    *pstr = NULL;

    /* unpack the flag */
    uint32_t flag;
    mfu_unpack_uint32(pptr, &flag);

    /* if we have a string, unpack the string */
    if (flag) {
        /* make a copy of the string to return to caller */
        const char* str = *pptr;
        *pstr = MFU_STRDUP(str);

        /* advance caller's pointer */
        size_t len = strlen(str) + 1;
        *pptr += len;
    }

    return;
}

/* given a pointer to a mfu_param_path, compute number of bytes
 * needed to pack this into a buffer */
static size_t mfu_pack_param_size(const mfu_param_path* param)
{
    size_t bytes = 0;

    /* flag to indicate whether orig path is defined */
    bytes += mfu_pack_str_size(param->orig);

    /* flag to indicate whether reduced path is defined */
    bytes += mfu_pack_str_size(param->path);

    /* record reduced path and stat data if we have it */
    if (param->path != NULL) {
        /* flag for stat data (uint32_t) */
        bytes += 4;

        /* bytes to hold stat data for item */
        if (param->path_stat_valid) {
            bytes += mfu_pack_stat_size();
        }
    }

    /* flag to indicate whether target path exists */
    bytes += mfu_pack_str_size(param->target);

    /* record target path and stat data if we have it */
    if (param->target != NULL) {
        /* flag for stat data (uint32_t) */
        bytes += 4;

        /* bytes to hold target stat data */
        if (param->target_stat_valid) {
            bytes += mfu_pack_stat_size();
        }
    }

    return bytes;
}

/* given a pointer to a mfu_param_path, pack into the specified
 * buffer and update the pointer to point to next free byte */
static void mfu_pack_param(char** pptr, const mfu_param_path* param)
{
    /* pack the original path */
    mfu_pack_str(pptr, param->orig);

    /* pack the reduced path and stat data */
    mfu_pack_str(pptr, param->path);

    /* pack the reduced path stat data */
    if (param->path != NULL) {
        if (param->path_stat_valid) {
            /* set flag to indicate that we have stat data */
            mfu_pack_uint32(pptr, 1);
    
            /* pack stat data */
            mfu_pack_stat(pptr, &param->path_stat);
        } else {
            /* set flag to indicate that we don't have stat data */
            mfu_pack_uint32(pptr, 0);
        }
    }

    /* pack the target path and stat data */
    mfu_pack_str(pptr, param->target);

    /* pack the target stat data */
    if (param->target != NULL) {
        if (param->target_stat_valid) {
            /* set flag to indicate that we have stat data */
            mfu_pack_uint32(pptr, 1);

            /* pack stat data */
            mfu_pack_stat(pptr, &param->target_stat);
        } else {
            /* set flag to indicate that we don't have stat data */
            mfu_pack_uint32(pptr, 0);
        }
    }

    return;
}

static void mfu_unpack_param(const char** pptr, mfu_param_path* param)
{
    /* initialize all values */
    mfu_param_path_init(param);

    /* unpack original path */
    mfu_unpack_str(pptr, &param->orig);

    /* unpack reduced path */
    mfu_unpack_str(pptr, &param->path);

    /* unpack reduce path stat data */
    if (param->path != NULL) {
        uint32_t val;
        mfu_unpack_uint32(pptr, &val);
        param->path_stat_valid = (int) val;

        if (param->path_stat_valid) {
            mfu_unpack_stat(pptr, &param->path_stat);
        }
    }

    /* unpack target path */
    mfu_unpack_str(pptr, &param->target);

    /* unpack target path stat data */
    if (param->target != NULL) {
        uint32_t val;
        mfu_unpack_uint32(pptr, &val);
        param->target_stat_valid = (int) val;

        if (param->target_stat_valid) {
            mfu_unpack_stat(pptr, &param->target_stat);
        }
    }

    return;
}

void mfu_param_path_set_all(uint64_t num, const char** paths, mfu_param_path* params)
{
    /* get our rank and number of ranks */
    int rank, ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);

    /* determine number we should look up */
    uint64_t count = num / (uint64_t) ranks;
    uint64_t extra = num - count * (uint64_t) ranks;
    if (rank < (int) extra) {
        /* procs whose rank is less than extra each
         * handle one extra param than those whose
         * rank is equal or greater than extra */
        count++;
    }

    /* determine our starting point */
    uint64_t start = 0;
    if (rank < (int) extra) {
        /* for these procs, count is one more than procs with ranks >= extra */
        start = (uint64_t)rank * count;
    } else {
        /* for these procs, count is one less than procs with ranks < extra */
        start = extra * (count + 1) + ((uint64_t)rank - extra) * count;
    }

    /* TODO: allocate temporary params */
    mfu_param_path* p = MFU_MALLOC(count * sizeof(mfu_param_path));

    /* track maximum path length */
    uint64_t bytes = 0;

    /* process each path we're responsible for */
    uint64_t i;
    for (i = 0; i < count; i++) {
        /* get pointer to param structure */
        mfu_param_path* param = &p[i];

        /* initialize all fields */
        mfu_param_path_init(param);

        /* lookup the path */
        uint64_t path_idx = start + i;
        const char* path = paths[path_idx];

        /* set param fields for path */
        if (path != NULL) {
            /* make a copy of original path */
            param->orig = MFU_STRDUP(path);

            /* get absolute path and remove ".", "..", consecutive "/",
             * and trailing "/" characters */
            param->path = mfu_path_strdup_abs_reduce_str(path);

            /* get stat info for simplified path */
            if (mfu_lstat(param->path, &param->path_stat) == 0) {
                param->path_stat_valid = 1;
            }

            /* TODO: we use realpath below, which is nice since it takes out
             * ".", "..", symlinks, and adds the absolute path, however, it
             * fails if the file/directory does not already exist, which is
             * often the case for dest path. */

            /* resolve any symlinks */
            char target[PATH_MAX];
            if (realpath(path, target) != NULL) {
                /* make a copy of resolved name */
                param->target = MFU_STRDUP(target);

                /* get stat info for resolved path */
                if (mfu_lstat(param->target, &param->target_stat) == 0) {
                    param->target_stat_valid = 1;
                }
            }

            /* add in bytes needed to pack this param */
            bytes += (uint64_t) mfu_pack_param_size(param);
        }
    }

    /* TODO: eventually it would be nice to leave this data distributed,
     * however for now some tools expect all params to be defined */

    /* allgather to get bytes on each process */
    int* recvcounts = (int*) MFU_MALLOC(ranks * sizeof(int));
    int* recvdispls = (int*) MFU_MALLOC(ranks * sizeof(int));
    int sendcount = (int) bytes;
    MPI_Allgather(&sendcount, 1, MPI_INT, recvcounts, 1, MPI_INT, MPI_COMM_WORLD);

    /* compute displacements and total number of bytes that we'll receive */
    uint64_t allbytes = 0;
    int disp = 0;
    for (i = 0; i < (uint64_t) ranks; i++) {
        recvdispls[i] = disp;
        disp += recvcounts[i];
        allbytes += (uint64_t) recvcounts[i];
    }

    /* allocate memory for send and recv buffers */
    char* sendbuf = MFU_MALLOC(bytes);
    char* recvbuf = MFU_MALLOC(allbytes);

    /* pack send buffer */
    char* ptr = sendbuf;
    for (i = 0; i < count; i++) {
        mfu_pack_param(&ptr, &p[i]);
    }

    /* allgatherv to collect data */
    MPI_Allgatherv(sendbuf, sendcount, MPI_BYTE, recvbuf, recvcounts, recvdispls, MPI_BYTE, MPI_COMM_WORLD);

    /* unpack recv buffer into caller's params */
    ptr = recvbuf;
    for (i = 0; i < num; i++) {
        mfu_unpack_param(&ptr, &params[i]);
    }

    /* Loop through the list of files &/or directories, and check the params 
     * struct to see if all of them are valid file names. If one is not, let 
     * the user know by printing a warning */
    if (rank == 0) {
        for (i = 0; i < num; i++) {
            /* get pointer to param structure */
            mfu_param_path* param = &p[i];
            if (param->path_stat_valid == 0) {
                /* failed to find a file at this location, let user know (may be a typo) */
                printf("Warning: `%s' does not exist\n", param->orig); 
            }
        }
    }

    /* free message buffers */
    mfu_free(&recvbuf);
    mfu_free(&sendbuf);

    /* free arrays for recv counts and displacements */
    mfu_free(&recvdispls);
    mfu_free(&recvcounts);

    /* free temporary params */
    mfu_free(&p);

    return;
}

/* free resources allocated in call to mfu_param_path_set_all */
void mfu_param_path_free_all(uint64_t num, mfu_param_path* params)
{
    /* make sure we got a valid pointer */
    if (params != NULL) {
        /* free memory for each param */
        uint64_t i;
        for (i = 0; i < num; i++) {
            /* get pointer to param structure */
            mfu_param_path* param = &params[i];

            /* free memory and reinit */
            if (param != NULL) {
                /* free all mememory */
                mfu_free(&param->orig);
                mfu_free(&param->path);
                mfu_free(&param->target);

                /* initialize all fields */
                mfu_param_path_init(param);
            }
        }
    }
    return;
}

/* set fields in param according to path */
void mfu_param_path_set(const char* path, mfu_param_path* param)
{
    mfu_param_path_set_all(1, &path, param);
    return;
}

/* free memory associated with param */
void mfu_param_path_free(mfu_param_path* param)
{
    mfu_param_path_free_all(1, param);
    return;
}

/* given a list of param_paths, walk each one and add to flist */
void mfu_param_path_walk(uint64_t num, const mfu_param_path* params, int walk_stat, mfu_flist flist, int dir_perms)
{
    /* allocate memory to hold a list of paths */
    const char** path_list = (const char**) MFU_MALLOC(num * sizeof(char*));

    /* fill list of paths and print each one */
    uint64_t i;
    for (i = 0; i < num; i++) {
        /* get path for this step */
        path_list[i] = params[i].path;
    }

    /* walk file tree and record stat data for each file */
    mfu_flist_walk_paths((uint64_t) num, path_list, walk_stat, dir_perms, flist);

    /* free the list */
    mfu_free(&path_list);

    return;
}
