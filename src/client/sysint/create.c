/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/* Create Function Implementation */

#include <assert.h>

#include "pinode-helper.h"
#include "pvfs2-sysint.h"
#include "pint-sysint-utils.h"
#include "pint-dcache.h"
#include "pint-servreq.h"
#include "pint-bucket.h"
#include "pcache.h"
#include "PINT-reqproto-encode.h"
#include "pvfs-distribution.h"

extern struct server_configuration_s g_server_config;

/* PVFS_sys_create()
 *
 * create a PVFS file with specified attributes 
 *
 * DEPRECATED: SEE sys-create.sm FOR CURRENT IMPLEMENTATION.
 *
 * returns 0 on success, -errno on failure
 */
int PVFS_sys_create_old(
    char* entry_name,
    PVFS_pinode_reference parent_refn,
    PVFS_sys_attr attr,
    PVFS_credentials credentials,
    PVFS_sysresp_create *resp)
{
	struct PVFS_server_req req_p;			/* server request */
	struct PVFS_server_resp *ack_p = NULL;	/* server response */
	int ret = -1, io_serv_count = 0, i = 0;
	int attr_mask, last_handle_created = 0;
	PINT_pinode *parent_ptr = NULL, *pinode_ptr = NULL;
	bmi_addr_t serv_addr1,serv_addr2,*bmi_addr_list = NULL;
	PVFS_handle *df_handle_array = NULL;
	PVFS_pinode_reference entry;
	struct PINT_decoded_msg decoded;
	void* encoded_resp;
	PVFS_msg_tag_t op_tag;
	bmi_size_t max_msg_sz;
	int old_ret = -1;
        PVFS_handle_extent_array *io_handle_extent_array = NULL;

	enum {
	    NONE_FAILURE = 0,
	    PCACHE_LOOKUP_FAILURE,
	    DCACHE_LOOKUP_FAILURE,
	    DCACHE_INSERT_FAILURE,
	    LOOKUP_SERVER_FAILURE,
	    CREATE_MSG_FAILURE,
	    CRDIRENT_MSG_FAILURE,
	    PREIO1_CREATE_FAILURE,
	    PREIO2_CREATE_FAILURE,
	    PREIO3_CREATE_FAILURE,
	    IO_REQ_FAILURE,
	    SETATTR_FAILURE,
	    PCACHE_INSERT1_FAILURE,
	    PCACHE_INSERT2_FAILURE,
	} failure = NONE_FAILURE;

	if((attr.mask & PVFS_ATTR_SYS_ALL_SETABLE) != PVFS_ATTR_SYS_ALL_SETABLE)
	{
	    gossip_lerr("Error: PVFS_sys_create(): failed to specify manditory attribute fields.\n");
	    return(-EINVAL);
	}

	/* make sure we recieved sane arguments */
	if (resp==NULL)
	{
	    return -EINVAL;
	}
	if((strlen(entry_name) + 1) > PVFS_REQ_LIMIT_SEGMENT_BYTES) 
	{
	    return -ENAMETOOLONG;
	}

	gossip_ldebug(CLIENT_DEBUG,"creating file named %s\n", entry_name);
	gossip_ldebug(CLIENT_DEBUG,"parent handle = %lld\n", parent_refn.handle);
	gossip_ldebug(CLIENT_DEBUG,"parent fsid = %d\n", parent_refn.fs_id);

        /* get the pinode of the parent so we can check permissions */
        attr_mask = PVFS_ATTR_COMMON_ALL;
        ret = phelper_get_pinode(parent_refn, &parent_ptr,
                                 attr_mask, credentials);
        if (ret < 0)
        {
	    /* parent pinode doesn't exist ?!? */
	    gossip_ldebug(CLIENT_DEBUG,"unable to get pinode for parent\n");
	    failure = PCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}
        assert(parent_ptr);

	/* check permissions in parent directory */
	ret = PINT_check_perms(parent_ptr->attr, parent_ptr->attr.perms,
                          credentials.uid, credentials.gid);
	if (ret < 0)
	{
	    phelper_release_pinode(parent_ptr);
	    ret = (-EPERM);
	    gossip_ldebug(CLIENT_DEBUG,"--===PERMISSIONS===--\n");
	    failure = PCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}

	/* also make sure that the parent is a directory */
	if(parent_ptr->attr.objtype != PVFS_TYPE_DIRECTORY)
	{
	    phelper_release_pinode(parent_ptr);
	    ret = (-PVFS_ENOTDIR);
	    failure = PCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}

	/* we're done with the parent pinode pointer */
        phelper_release_pinode(parent_ptr);

	/* Lookup handle(if it exists) in dcache */
	ret = PINT_dcache_lookup(entry_name,parent_refn,&entry);
	if (ret < 0 && ret != -PVFS_ENOENT)
	{
	    /* there was an error, bail*/
	    failure = DCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}
	else if (ret == 0)
	{
	    /* pinode already exists, should fail create with EXISTS*/
	    ret = (-PVFS_EEXIST);
	    failure = DCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}

	/* otherwise we were returned -PVFS_ENOENT, indicating that
	 * there was no match in the dcache.
	 */

	/* how many data files do we need to create? */
	/* get default value from system */
	ret = PINT_bucket_get_num_io(parent_refn.fs_id, &io_serv_count);
	if(ret < 0)
	{
	    failure = DCACHE_LOOKUP_FAILURE;
	    goto return_error;
	}

	/* right now, there is no way to request a certain number of 
	 * data files- just go with the system default. 
	 */
	PINT_bucket_get_num_io( parent_refn.fs_id, &io_serv_count);

	/* but make sure we don't exceed the request protocol limit */
	if(io_serv_count > PVFS_REQ_LIMIT_DFILE_COUNT)
	{
	    io_serv_count = PVFS_REQ_LIMIT_DFILE_COUNT;
	    gossip_ldebug(CLIENT_DEBUG, "Warning: reducing number of "
                          "data files to PVFS_REQ_LIMIT_DFILE_COUNT.\n");
	}

	gossip_ldebug(CLIENT_DEBUG,"number of data files to create "
                      "= %d\n",io_serv_count);

	/*
          Determine the initial metaserver and the appropriate
          handle range for a new file.  This extent array of
          handles should NOT be freed.
        */
	ret = PINT_bucket_get_next_meta(&g_server_config,
                                        parent_refn.fs_id,
                                        &serv_addr1,
                                        &req_p.u.create.handle_extent_array);
	if (ret < 0)
	{
	    failure = LOOKUP_SERVER_FAILURE;
	    goto return_error;
	}
	
	/* send the create request for the meta file */
	req_p.op = PVFS_SERV_CREATE;
	req_p.credentials = credentials;
        /*
          NOTE: handle_extent_array is filled in above in the call
          to PINT_bucket_get_next_meta since we don't need a specific
          handle value, but rather any valid one on that meta server.
        */
	req_p.u.create.fs_id = parent_refn.fs_id;

	/* Q: is this sane?  pretty sure we're creating meta files here, but do 
	 * we want to re-use this for other object types? symlinks, dirs, etc?*/

	req_p.u.create.object_type = PVFS_TYPE_METAFILE;

	max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
	    PINT_CLIENT_ENC_TYPE);

	op_tag = get_next_session_tag();
	/* send the server request */
	ret = PINT_send_req(serv_addr1, &req_p, max_msg_sz,
            &decoded, &encoded_resp, op_tag);
	if (ret < 0)
	{
	    failure = LOOKUP_SERVER_FAILURE;
	    goto return_error;
	}

	ack_p = (struct PVFS_server_resp *) decoded.buffer;
	if (ack_p->status < 0 )
	{
	    ret = ack_p->status;
	    failure = CREATE_MSG_FAILURE;
	    goto return_error;
        }

	/* save the handle for the meta file so we can refer to it later */
	entry.handle = ack_p->u.create.handle;
	entry.fs_id = parent_refn.fs_id;

	/* these fields are the only thing we need to set for the response to
	 * the calling function
	 */

	resp->pinode_refn = entry;

	PINT_release_req(serv_addr1, &req_p, max_msg_sz, &decoded,
            &encoded_resp, op_tag);

	/* Create directory entry server request to parent */
	/* Query BTI to get initial meta server */
	ret = PINT_bucket_map_to_server(&serv_addr2,parent_refn.handle,
                                        parent_refn.fs_id);
	if (ret < 0)
	{
	    failure = CREATE_MSG_FAILURE;
	    goto return_error;
	}

	/* send crdirent to associate a name with the meta file we just made */
	req_p.op = PVFS_SERV_CREATEDIRENT;

	/* credentials come from credentials and are set in the previous
	 * create request.  so we don't have to set those again.
	 */

	/* just update the pointer, it'll get malloc'ed when its sent on the 
	 * wire.
	 */
	req_p.u.crdirent.name = entry_name;
	req_p.u.crdirent.new_handle = entry.handle;
	req_p.u.crdirent.parent_handle = parent_refn.handle;
	req_p.u.crdirent.fs_id = parent_refn.fs_id;

	/* max response size is the same as the previous request */

	op_tag = get_next_session_tag();

	/* Make server request */
	ret = PINT_send_req(serv_addr2, &req_p, max_msg_sz,
            &decoded, &encoded_resp, op_tag);
	if (ret < 0)
	{
	    failure = CREATE_MSG_FAILURE;
	    goto return_error;
	}

	ack_p = (struct PVFS_server_resp *) decoded.buffer;
	if (ack_p->status < 0 )
        {
	    /* this could fail for many reasons, EEXISTS will probbably be the 
	     * most common.
	     */
            ret = ack_p->status;
            failure = CRDIRENT_MSG_FAILURE;
            goto return_error;
        }

	PINT_release_req(serv_addr2, &req_p, max_msg_sz, &decoded,
            &encoded_resp, op_tag);

	/* we need one BMI address for each data file */
	bmi_addr_list = (bmi_addr_t *)malloc(sizeof(bmi_addr_t)*io_serv_count);
	if (bmi_addr_list == NULL)
	{
		ret = (-ENOMEM);
		failure = PREIO1_CREATE_FAILURE;
		goto return_error;
	}

	/* I'm using df_handle_array to store the new bucket # before I 
	 * send the create request and then the newly created handle when I
	 * get that back from the server.
	 *
	 * this may be confusing:  since we don't care about the bucket after
	 * sending the create request, I'm reusing this space to store the new
	 * handle that we get back from the server. its cheaper since I only
	 * malloc once.  Both items are stored as PVFS_handle types.
	 * 
	 */

	df_handle_array = (PVFS_handle*)
            malloc(io_serv_count*sizeof(PVFS_handle));
	if (df_handle_array == NULL)
	{
	    failure = PREIO2_CREATE_FAILURE;
	    goto return_error;
	}
        memset(df_handle_array,0,io_serv_count*sizeof(PVFS_handle));

	/* we need one data handle range for each data file handle */
        io_handle_extent_array = (PVFS_handle_extent_array *)
            malloc(io_serv_count * sizeof(PVFS_handle_extent_array));
        if (!io_handle_extent_array)
        {
            gossip_err("Cannot allocate handle extent array\n");
            failure = PREIO3_CREATE_FAILURE;
            goto return_error;
        }

	ret = PINT_bucket_get_next_io(&g_server_config,
                                      parent_refn.fs_id,
                                      io_serv_count,
                                      bmi_addr_list,
                                      io_handle_extent_array);
	if (ret < 0)
	{
            gossip_err("Failed to get I/O server configuration\n");
	    failure = PREIO3_CREATE_FAILURE;
	    goto return_error;
	}

	/* send create requests to each I/O server for the data files */

	/* TODO: right now this is serialized, we should come back later and 
	 * make this asynchronous or something.
	 */

	/* these fields are the same for each server message so we only need to
	 * set them once.
	 */

	req_p.op = PVFS_SERV_CREATE;
	req_p.credentials = credentials;
	req_p.u.create.fs_id = parent_refn.fs_id;
	/* we're making data files on each server */
	req_p.u.create.object_type = PVFS_TYPE_DATAFILE;

	/* create requests get a generic response */
	max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
	    PINT_CLIENT_ENC_TYPE);

	/* NOTE: if we need to rollback data file creation, the first valid
	 * handle to remove would be i - 1 (as long as i < 0).
	 */

	for(i = 0;i < io_serv_count; i++)
	{
            assert(io_handle_extent_array[i].extent_count > 0);
            assert(io_handle_extent_array[i].extent_array);

            req_p.u.create.handle_extent_array.extent_count =
                io_handle_extent_array[i].extent_count;
            req_p.u.create.handle_extent_array.extent_array =
                io_handle_extent_array[i].extent_array;

            op_tag = get_next_session_tag();

            ret = PINT_send_req(bmi_addr_list[i], &req_p, max_msg_sz,
                                &decoded, &encoded_resp, op_tag);
            if (ret < 0)
            {
                /* if we fail then we assume no data file has been created
                 * on the server
                 */
                failure = IO_REQ_FAILURE;
                goto return_error;
            }

            ack_p = (struct PVFS_server_resp *) decoded.buffer;
            if (ack_p->status < 0 )
            {
                ret = ack_p->status;
                failure = IO_REQ_FAILURE;
                goto return_error;
            }

            /* store the new handle here */
            df_handle_array[i] = ack_p->u.create.handle;

            PINT_release_req(bmi_addr_list[i], &req_p, max_msg_sz,
                             &decoded, &encoded_resp, op_tag);
	}

	/* store all the handles to the files we've created in the metafile
	 * on the server. (via setattr).
	 */
	req_p.op = PVFS_SERV_SETATTR;
	req_p.u.setattr.handle = entry.handle;
	req_p.u.setattr.fs_id = parent_refn.fs_id;

	PINT_CONVERT_ATTR(&req_p.u.setattr.attr, &attr, PVFS_ATTR_COMMON_ALL);
	req_p.u.setattr.attr.u.meta.dfile_array = df_handle_array;
	req_p.u.setattr.attr.u.meta.dfile_count = io_serv_count;
	req_p.u.setattr.attr.u.meta.dist = PVFS_Dist_create("simple_stripe");
	req_p.u.setattr.attr.objtype = PVFS_TYPE_METAFILE;

	gossip_ldebug(CLIENT_DEBUG,"\towner: %d\n\tgroup: %d\n\tperms: %d\n\tatime: %lld\n\tmtime: %lld\n\tctime: %lld\n\tobjtype: %d\n",
		req_p.u.setattr.attr.owner, 
		req_p.u.setattr.attr.group, 
		req_p.u.setattr.attr.perms, 
		(long long)req_p.u.setattr.attr.atime, 
		(long long)req_p.u.setattr.attr.mtime, 
		(long long)req_p.u.setattr.attr.ctime, 
		req_p.u.setattr.attr.objtype);
	gossip_ldebug(CLIENT_DEBUG,"\t\tdfile_count: %d\n",
		(int)req_p.u.setattr.attr.u.meta.dfile_count);

    for(i=0;i<req_p.u.setattr.attr.u.meta.dfile_count;i++)
	gossip_ldebug(CLIENT_DEBUG,"\t\tdatafile handle: %lld\n",
		req_p.u.setattr.attr.u.meta.dfile_array[i]);

	/* set the type of the object */
	req_p.u.setattr.attr.objtype = PVFS_TYPE_METAFILE;
	/* we want to set whatever fields the caller specified, 
	 * plus the object type, array of datafiles, and distribution
	 */
	req_p.u.setattr.attr.mask |= PVFS_ATTR_COMMON_TYPE;
	req_p.u.setattr.attr.mask |= PVFS_ATTR_META_ALL;

	max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
	    PINT_CLIENT_ENC_TYPE);

	op_tag = get_next_session_tag();

	/* send the setattr request to the meta server */
	ret = PINT_send_req(serv_addr1, &req_p, max_msg_sz,
            &decoded, &encoded_resp, op_tag);
	if (ret < 0)
	{
	    failure = SETATTR_FAILURE;
	    goto return_error;
	}

	ack_p = (struct PVFS_server_resp *) decoded.buffer;

	/* make sure the operation didn't fail*/
	if (ack_p->status < 0 )
	{
	    ret = ack_p->status;
	    failure = SETATTR_FAILURE;
	    goto return_error;
	}
	PINT_release_req(serv_addr1, &req_p, max_msg_sz, &decoded,
	    &encoded_resp, op_tag);

	/* don't need to hold on to the io server addresses anymore */
	free(bmi_addr_list);

	/* add entry to dcache */
	ret = PINT_dcache_insert(entry_name, entry, parent_refn);
	if (ret < 0)
	{
	    failure = DCACHE_INSERT_FAILURE;
	    goto return_error;
	}

        pinode_ptr = PINT_pcache_lookup(entry);
        if (!pinode_ptr)
        {
            pinode_ptr = PINT_pcache_pinode_alloc();
            assert(pinode_ptr);
        }
	pinode_ptr->refn = entry;
	pinode_ptr->attr = req_p.u.setattr.attr;
	pinode_ptr->attr.objtype = PVFS_TYPE_METAFILE;
	pinode_ptr->attr.mask |= PVFS_ATTR_COMMON_TYPE;

        PINT_pcache_set_valid(pinode_ptr);
  	return(0); 

return_error:
	switch(failure)
	{
	    case PCACHE_INSERT2_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PCACHE_INSERT2_FAILURE\n");
	    case PCACHE_INSERT1_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PCACHE_INSERT1_FAILURE\n");
	    case DCACHE_INSERT_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"DCACHE_INSERT_FAILURE\n");
		ret = 0;
		break;
	    case SETATTR_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"SETATTR_FAILURE\n");
	    case IO_REQ_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"IO_REQ_FAILURE\n");
		/* rollback each of the data files we created */
		last_handle_created = i;
		req_p.op = PVFS_SERV_REMOVE;
		req_p.credentials = credentials;
		req_p.u.remove.fs_id = parent_refn.fs_id;
		max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
		    PINT_CLIENT_ENC_TYPE);

		for(i = 0;i < last_handle_created; i++)
		{
		    /*best effort rollback*/
		    req_p.u.remove.handle = df_handle_array[i];
		    op_tag = get_next_session_tag();
		    old_ret = ret;
		    ret = PINT_send_req(bmi_addr_list[i], &req_p, max_msg_sz,
			&decoded, &encoded_resp, op_tag);
		    if(ret == 0)
			PINT_release_req(bmi_addr_list[i], &req_p, max_msg_sz, &decoded,
			    &encoded_resp, op_tag);
		    ret = old_ret;
		}

	    case PREIO3_CREATE_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PREIO3_CREATE_FAILURE\n");
		free(df_handle_array);
	    case PREIO2_CREATE_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PREIO2_CREATE_FAILURE\n");
		free(bmi_addr_list);
	    case PREIO1_CREATE_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PREIO1_CREATE_FAILURE\n");
		/* rollback crdirent */
		req_p.op = PVFS_SERV_RMDIRENT;

		req_p.credentials = credentials;
		req_p.u.rmdirent.parent_handle = parent_refn.handle;
		req_p.u.rmdirent.fs_id = parent_refn.fs_id;
		req_p.u.rmdirent.entry = entry_name;
		max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
		    PINT_CLIENT_ENC_TYPE);
		op_tag = get_next_session_tag();
		old_ret = ret;
                ret = PINT_send_req(serv_addr2, &req_p, max_msg_sz,
                    &decoded, &encoded_resp, op_tag);
		if(ret == 0)
		    PINT_release_req(serv_addr2, &req_p, max_msg_sz, &decoded,
			&encoded_resp, op_tag);
		ret = old_ret;

	    case CRDIRENT_MSG_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"CRDIRENT_MSG_FAILURE\n");
		/* rollback create req*/
		req_p.op = PVFS_SERV_REMOVE;
		req_p.credentials = credentials;
		req_p.u.remove.handle = entry.handle;
		req_p.u.remove.fs_id = entry.fs_id;
		max_msg_sz = PINT_encode_calc_max_size(PINT_ENCODE_RESP, req_p.op,
		    PINT_CLIENT_ENC_TYPE);
		op_tag = get_next_session_tag();
		old_ret = ret;
                ret = PINT_send_req(serv_addr1, &req_p, max_msg_sz,
                    &decoded, &encoded_resp, op_tag);
		if(ret == 0)
		    PINT_release_req(serv_addr1, &req_p, max_msg_sz, &decoded,
			 &encoded_resp, op_tag);
		ret = old_ret;
		/* NOTE: setting buffer to NULL to keep the next
		 * recovery case from trying to release it.
		 */
		decoded.buffer = NULL;

	    case CREATE_MSG_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"CREATE_MSG_FAILURE\n");

		/*op_tag should still be valid since we just failed*/
		if (decoded.buffer != NULL)
		    PINT_release_req(serv_addr1, &req_p, max_msg_sz, &decoded,
			&encoded_resp, op_tag);

	    case LOOKUP_SERVER_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"LOOKUP_SERVER_FAILURE\n");
	    case PCACHE_LOOKUP_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"PCACHE_LOOKUP_FAILURE\n");
	    case DCACHE_LOOKUP_FAILURE:
		gossip_ldebug(CLIENT_DEBUG,"DCACHE_LOOKUP_FAILURE\n");
	    case NONE_FAILURE:

	    /* TODO: do we want to setup a #define for these invalid handle/fsid
	     * values? */
	    resp->pinode_refn.handle = -1;
	    resp->pinode_refn.fs_id = -1;
	}
	return (ret);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
