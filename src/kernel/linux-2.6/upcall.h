/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#ifndef __UPCALL_H
#define __UPCALL_H

/* TODO: we might want to try to avoid this inclusion  */
#include "pvfs2-sysint.h"

typedef struct
{
    char *buf;
    size_t count;
    loff_t offset;
    PVFS_pinode_reference refn;
    enum PVFS_io_type io_type;
} pvfs2_io_request_t;

typedef struct
{
    PVFS_pinode_reference parent_refn;
    char d_name[PVFS2_NAME_LEN];
} pvfs2_lookup_request_t;

typedef struct
{
    PVFS_pinode_reference parent_refn;
    PVFS_sys_attr attributes;
    char d_name[PVFS2_NAME_LEN];
} pvfs2_create_request_t;

typedef struct
{
    PVFS_pinode_reference refn;
} pvfs2_getattr_request_t;

typedef struct
{
    PVFS_pinode_reference refn;
    PVFS_sys_attr attributes;
} pvfs2_setattr_request_t;

typedef struct
{
    PVFS_pinode_reference parent_refn;
    char d_name[PVFS2_NAME_LEN];
} pvfs2_remove_request_t;

typedef struct
{
    PVFS_pinode_reference parent_refn;
    PVFS_sys_attr attributes;
    char d_name[PVFS2_NAME_LEN];
} pvfs2_mkdir_request_t;

typedef struct
{
    PVFS_pinode_reference refn;
    PVFS_ds_position token;
    int max_dirent_count;
} pvfs2_readdir_request_t;

typedef struct
{
    int type;
    PVFS_credentials credentials;

    union
    {
	pvfs2_io_request_t io;
	pvfs2_lookup_request_t lookup;
	pvfs2_create_request_t create;
	pvfs2_getattr_request_t getattr;
	pvfs2_setattr_request_t setattr;
	pvfs2_remove_request_t remove;
	pvfs2_mkdir_request_t mkdir;
	pvfs2_readdir_request_t readdir;
    } req;
} pvfs2_upcall_t;

#endif /* __UPCALL_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
