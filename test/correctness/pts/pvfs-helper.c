#include "pvfs-helper.h"

pvfs_helper_t pvfs_helper;

extern int parse_pvfstab(char *fn,  pvfs_mntlist *mnt);

int initialize_sysint()
{
    int ret = -1;

    memset(&pvfs_helper,0,sizeof(pvfs_helper));

    ret = parse_pvfstab(NULL,&pvfs_helper.mnt);
    if (ret > -1)
    {
        gossip_disable();

        /* init the system interface */
        ret = PVFS_sys_initialize(pvfs_helper.mnt,
                                  &pvfs_helper.resp_init);
        if(ret > -1)
        {
            pvfs_helper.initialized = 1;
            pvfs_helper.num_test_files = NUM_TEST_FILES;
            ret = 0;
        }
        else
        {
            fprintf(stderr, "Error: PVFS_sys_initialize() "
                    "failure. = %d\n", ret);
        }
    }
    else
    {
        fprintf(stderr, "Error: parse_pvfstab() failure.\n");
    }
    return ret;
}
