/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/* Set Attribute Function Implementation */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "pinode-helper.h"
#include "pvfs2-sysint.h"
#include "pinode-helper.h"
#include "pint-sysint.h"
#include "pint-servreq.h"
#include "pint-bucket.h"
#include "PINT-reqproto-encode.h"

/* PVFS_sys_setattr()
 *
 * sets attributes for a particular PVFS file 
 *
 * returns 0 on success, -errno on failure
 */
int PVFS_sys_setattr(PVFS_pinode_reference pinode_refn, PVFS_object_attr attr,
                PVFS_credentials credentials)
{
	struct PVFS_server_req req_p;			/* server request */
	struct PVFS_server_resp *ack_p = NULL;	/* server response */
	int ret = -1;
	pinode *pinode_ptr = NULL;
	bmi_addr_t serv_addr;		/* PVFS address type structure */
	char *server = NULL;
	PVFS_pinode_reference entry;
	PVFS_size handlesize = 0;
	bmi_size_t max_msg_sz = PINT_get_encoded_generic_ack_sz(0, PVFS_SERV_SETATTR);
	struct PINT_decoded_msg decoded;
	void* encoded_resp;
	PVFS_msg_tag_t op_tag = get_next_session_tag();
	int pinode_was_in_cache = 1;

	enum {
	    NONE_FAILURE = 0,
	    PCACHE_LOOKUP_FAILURE,
	    PINODE_REMOVE_FAILURE,
	    MAP_TO_SERVER_FAILURE,
	} failure = NONE_FAILURE;

	/* we don't allow the file size to be set here - that is
	 * the job of the truncate function.
	 */
	if (attr.mask & PVFS_ATTR_SYS_SIZE)
	    return (-EINVAL);

	/* Fill in pinode reference */
	entry.handle = pinode_refn.handle;
	entry.fs_id = pinode_refn.fs_id;
	
	/* Lookup the entry...may or may not exist in the cache */

	ret = PINT_pcache_lookup(entry, &pinode_ptr);
	/* Check if pinode was returned */
	if (ret == PCACHE_LOOKUP_FAILURE)
	{
		pinode_was_in_cache = 0;
		ret = phelper_get_pinode(entry, &pinode_ptr,
		    PVFS_ATTR_COMMON_ALL, credentials);
		if (ret < 0)
		{
		    failure = PCACHE_LOOKUP_FAILURE;
		    goto return_error;
		}
	}

	/* by this point we definately have a pinode cache entry */

	/* Get the server thru the BTI using the handle */
	ret = PINT_bucket_map_to_server(&serv_addr,entry.handle,entry.fs_id);
	if (ret < 0)
	{
	    failure = MAP_TO_SERVER_FAILURE;
	    goto return_error;
	}

	/* Create the server request */
	req_p.op = PVFS_SERV_SETATTR;
	req_p.credentials = credentials;
	if (attr.objtype == PVFS_TYPE_METAFILE)
	{
	    handlesize = attr.u.meta.dfile_count * sizeof(PVFS_handle);
	}
	else
	{
	    handlesize = 0;
	}
	req_p.u.setattr.handle = entry.handle;
	req_p.u.setattr.fs_id = entry.fs_id;
	/* let attributes fall through since PVFS_ATTR_SYS_xxx
	 * mask values match PVFS_ATTR_COMMON_xxx mask values for
	 * all of the attributes that are valid to set here
	 */
	req_p.u.setattr.attr = attr;
	/* but make sure the caller didn't set extra mask bits */
	assert((attr.mask & ~PVFS_ATTR_SYS_ALL_NOSIZE) == 0);

	/* Make a server setattr request */	
	ret = PINT_send_req(serv_addr, &req_p, max_msg_sz,
            &decoded, &encoded_resp, op_tag);
	if (ret < 0)
	{
	    failure = MAP_TO_SERVER_FAILURE;
	    goto return_error;
	}

	ack_p = (struct PVFS_server_resp *) decoded.buffer;

	/* make sure the actual IO suceeded */
	if (ack_p->status < 0 )
	{
	    ret = ack_p->status;
	    failure = PINODE_REMOVE_FAILURE;
	    goto return_error;
	}

	PINT_release_req(serv_addr, &req_p, max_msg_sz, &decoded,
            &encoded_resp, op_tag);

	/* Modify pinode to reflect changed attributes */
	ret = phelper_fill_attr(pinode_ptr,attr);
	if (ret < 0)
	{
		failure = PINODE_REMOVE_FAILURE;
		goto return_error;
	}

	if (pinode_was_in_cache)
	{
	    PINT_pcache_lookup_rls(pinode_ptr);
	}
	else
	{
	    phelper_release_pinode(pinode_ptr);
	}

	return(0);

return_error:

	switch( failure )
	{
	    case PINODE_REMOVE_FAILURE:
		if (ack_p != NULL)
		    PINT_release_req(serv_addr, &req_p, max_msg_sz, &decoded,
	                &encoded_resp, op_tag);

	    case MAP_TO_SERVER_FAILURE:
		if (server != NULL)
		    free(server);

		if (pinode_was_in_cache)
		{
		    PINT_pcache_lookup_rls(pinode_ptr);
		}
		else
		{
		    phelper_release_pinode(pinode_ptr);
		}

	    case PCACHE_LOOKUP_FAILURE:
		PINT_pcache_pinode_dealloc(pinode_ptr);

	    case NONE_FAILURE:
	    break;
	}

	return(ret);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
