/*
 * (C) 2002 Clemson University and The University of Chicago.
 *
 * See COPYING in top-level directory.
 */

#ifndef __PVFS_DISTRIBUTION_H
#define __PVFS_DISTRIBUTION_H

#include "pvfs2-types.h"

/* each particular distribution implementation will define this for itself */
typedef struct {
	PVFS_offset (*logical_to_physical_offset)(void* params,
                                                  uint32_t server_nr,
                                                  uint32_t server_ct,
                                                  PVFS_offset logical_offset);
	PVFS_offset (*physical_to_logical_offset)(void* params,
                                                  uint32_t server_nr,
                                                  uint32_t server_ct,
                                                  PVFS_offset physical_offset);
	PVFS_offset (*next_mapped_offset)(void* params,
                                          uint32_t server_nr,
                                          uint32_t server_ct,
                                          PVFS_offset logical_offset);
	PVFS_size (*contiguous_length)(void* params,
                                       uint32_t server_nr,
                                       uint32_t server_ct,
                                       PVFS_offset physical_offset);
	PVFS_size (*logical_file_size)(void* params,
                                       uint32_t server_ct,
                                       PVFS_size *psizes);
	void (*encode)(void* params, void *buffer);
	void (*decode)(void* params, void *buffer);
	void (*encode_lebf)(char **pptr, void* params);
	void (*decode_lebf)(char **pptr, void* params);
} PVFS_Dist_methods;

/* this struct is used to define a distribution to PVFS */
typedef struct PINT_dist_s {
	char *dist_name;
	int32_t name_size;
	int32_t param_size; 
	void *params;
	PVFS_Dist_methods *methods;
} PINT_dist;

#ifdef __PINT_REQPROTO_ENCODE_FUNCS_C
#define encode_PVFS_Dist(pptr,x) do { PINT_dist *px = *(x); \
    encode_string(pptr, &px->dist_name); \
    if (!px->methods) { \
	gossip_err("%s: encode_PVFS_Dist: methods is null\n", __func__); \
	exit(1); \
    } \
    (px->methods->encode_lebf) (pptr, px->params); \
} while (0)
#define decode_PVFS_Dist(pptr,x) do { PINT_dist tmp_dist; PINT_dist *px; \
    extern int PINT_Dist_lookup(PINT_dist *dist); \
    decode_string(pptr, &tmp_dist.dist_name); \
    tmp_dist.params = 0; \
    tmp_dist.methods = 0; \
    /* bizzare lookup function fills in most fields */ \
    PINT_Dist_lookup(&tmp_dist); \
    if (!tmp_dist.methods) { \
	gossip_err("%s: decode_PVFS_Dist: methods is null\n", __func__); \
	exit(1); \
    } \
    /* later routines assume dist is a big contiguous thing, do so */ \
    *(x) = px = decode_malloc(PINT_DIST_PACK_SIZE(&tmp_dist)); \
    memcpy(px, &tmp_dist, sizeof(*px)); \
    px->dist_name = (char *) px + roundup8(sizeof(*px)); \
    memcpy(px->dist_name, tmp_dist.dist_name, tmp_dist.name_size); \
    px->params = (void *)(px->dist_name + roundup8(px->name_size)); \
    (px->methods->decode_lebf) (pptr, px->params); \
} while (0)
#endif

extern PINT_dist *PVFS_dist_create(const char *name);
extern int PVFS_dist_free(PINT_dist *dist);
extern PINT_dist *PVFS_Dist_copy(const PINT_dist *dist);
extern int PINT_dist_getparams(void *buf, const PINT_dist *dist);
extern int PINT_dist_setparams(PINT_dist *dist, const void *buf);

/******** macros for access to dist struct ***********/

#define PINT_DIST_PACK_SIZE(d) \
 (roundup8(sizeof(*(d))) + roundup8((d)->name_size) + roundup8((d)->param_size))

#endif /* __PVFS_DISTRIBUTION_H */