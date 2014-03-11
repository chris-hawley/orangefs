/*
 * (C) 2002 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#ifndef __MKSPACE_H
#define __MKSPACE_H

#include "pvfs2-internal.h"
#include "trove.h"

#define PVFS2_MKSPACE_GOSSIP_VERBOSE 1
#define PVFS2_MKSPACE_STDERR_VERBOSE 2

int pvfs2_mkspace(char *data_path,
                  char *meta_path,
                  char *config_path,
                  char *collection,
                  TROVE_coll_id coll_id,
                  TROVE_handle root_handle,
                  TROVE_handle root_dirdata_handle,
                  PVFS_SID *root_sid_array,
                  int root_sid_count,
/* V3 nolonger needed */
#if 0
                  char *meta_handle_ranges,
                  char *data_handle_ranges,
#endif
                  int create_collection_only,
                  int verbose);

int pvfs2_rmspace(char *data_path,
                  char *meta_path,
                  char *config_path,
                  char *collection,
                  TROVE_coll_id coll_id,
                  int remove_collection_only,
                  int verbose);

#endif /* __MKSPACE_H */
