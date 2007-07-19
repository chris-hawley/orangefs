#include <pvfs2-request.h>
#include <pint-distribution.h>
#include <pint-request.h>
#include <pvfs2-types.h>

#define PVFS_DIST_TWOD_STRIPE_NAME "twod_stripe"
#define PVFS_DIST_TWOD_STRIPE_NAME_SIZE 14
#define PVFS_DIST_TWOD_STRIPE_DEFAULT_GROUPS 1
#define PVFS_DIST_TWOD_STRIPE_DEFAULT_STRIP_SIZE 65536
#define PVFS_DIST_TWOD_STRIPE_DEFAULT_FACTOR 5

/* 2d stripe distribution parameters */
struct PVFS_twod_stripe_params_s {
    uint32_t num_groups;
    PVFS_size strip_size;
    uint32_t group_strip_factor;	    /* Number of strips/server/group */
};
typedef struct PVFS_twod_stripe_params_s PVFS_twod_stripe_params;
    


