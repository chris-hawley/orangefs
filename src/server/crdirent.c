/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */

#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "state-machine.h"
#include "server-config.h"
#include "pvfs2-server.h"
#include "pvfs2-attr.h"
#include "job-consist.h"

static int crdirent_init(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_read_directory_entry_handle(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_read_parent_metadata(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_verify_parent_metadata(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_write_directory_entry(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_create_dirdata_dspace(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_write_dirdata_handle(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_send_bmi(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_cleanup(PINT_server_op *s_op, job_status_s *ret);
static int crdirent_release_posted_job(PINT_server_op *s_op, job_status_s *ret);
void crdirent_init_state_machine(void);

extern PINT_server_trove_keys_s Trove_Common_Keys[];

PINT_state_machine_s crdirent_req_s = 
{
	NULL,
	"crdirent_dirent",
	crdirent_init_state_machine
};

enum {
    ERROR_BADNAME = 4
};

%%

machine crdirent(init,
		 read_directory_entry_handle,
		 read_parent_metadata,
		 verify_parent_metadata,
		 write_directory_entry,
		 send,
		 cleanup,
		 create_dirdata_dspace,
		 write_dirdata_handle,
		 release)
{
	state init
	{
		run crdirent_init;
		ERROR_BADNAME => send;
		default => read_parent_metadata;
	}

	state read_parent_metadata
	{
		run crdirent_read_parent_metadata;
		success => verify_parent_metadata;
		default => send;
	}

	state verify_parent_metadata
	{
		run crdirent_verify_parent_metadata;
		success => read_directory_entry_handle;
		default => send;
	}
	
	state read_directory_entry_handle
	{
		run crdirent_read_directory_entry_handle;
		success => write_directory_entry;
		default => create_dirdata_dspace;
	}

	state write_directory_entry
	{
		run crdirent_write_directory_entry;
		default => send;
	}

	state create_dirdata_dspace
	{
		run crdirent_create_dirdata_dspace;
		success => write_dirdata_handle;
		default => send;
	}

	state write_dirdata_handle
	{
		run crdirent_write_dirdata_handle;
		success => write_directory_entry;
		default => send;
	}

	state send
	{
		run crdirent_send_bmi;
		default => release;
	}

	state release
	{
		run crdirent_release_posted_job;
		default => cleanup;
	}

	state cleanup
	{
		run crdirent_cleanup;
		default => init;
	}
}

%%

/*
 * Function: crdirent_init_state_machine
 *
 * Params:   void
 *
 * Pre:      None
 *
 * Post:     None
 *
 * Returns:  void
 *
 * Synopsis: Point the state machine at the array produced by the
 * state-comp preprocessor for crdirent
 *           
 */

void crdirent_init_state_machine(void)
{
    crdirent_req_s.state_machine = crdirent;
}

/*
 * Function: crdirent_init
 *
 * Synopsis: This function sets up the buffers in preparation for
 *           the trove operation to get the attribute structure
 *           used in check permissions.  Also runs the operation through
 *           the request scheduler for consistency.
 *
 */
static int crdirent_init(PINT_server_op *s_op,
			 job_status_s *ret)
{
    int job_post_ret;
    char *ptr;

    gossip_debug(SERVER_DEBUG, "crdirent state: init\n");

    /* verify input values -- some of this should be caught as legitimate errors later! */
    assert(s_op->req->u.crdirent.name != NULL);
    assert(s_op->req->u.crdirent.new_handle != 0);
    assert(s_op->req->u.crdirent.parent_handle != 0);

    gossip_debug(SERVER_DEBUG,
	    "  got crdirent for %s (with handle 0x%08Lx) in 0x%08Lx\n",
	    s_op->req->u.crdirent.name,
	    s_op->req->u.crdirent.new_handle,
	    s_op->req->u.crdirent.parent_handle);

    /* check for invalid characters in name */
    ptr = s_op->req->u.crdirent.name;
    while (*ptr != '\0' && *ptr != '/' && isprint(*ptr)) ptr++;

    if (*ptr != '\0') {
	/* found an invalid character -- report it and send error response */
	if (*ptr == '/') {
	    gossip_lerr("crdirent: error: invalid '/' character in name (%s); sending error response.\n",
			s_op->req->u.crdirent.name);
	}
	else {
	    gossip_lerr("crdirent: error: invalid unprintable character (value = 0x%x) in name; sending error response.\n",
			(int) *ptr);
	}
	ret->error_code = ERROR_BADNAME;
	s_op->scheduled_id = 0;
	return 1;
    }

    /* post a scheduler job */
    job_post_ret = job_req_sched_post(s_op->req,
				      s_op,
				      ret,
				      &(s_op->scheduled_id));
    return job_post_ret;
}


/*
 * Function: crdirent_read_parent_metadata
 *
 * Synopsis: Post Trove job to read metadata for the parent directory so we can make sure
 *           that the credentials are valid.
 */
static int crdirent_read_parent_metadata(PINT_server_op *s_op,
					 job_status_s *ret)
{

    int job_post_ret;
    job_id_t i;

    gossip_ldebug(SERVER_DEBUG,"crdirent state: read_parent_metadata\n");

    assert(s_op->scheduled_id != 0);

    s_op->key.buffer    = Trove_Common_Keys[METADATA_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[METADATA_KEY].size;

    /* store parent attributes in crdirent scratch space */
    s_op->val.buffer    = &s_op->u.crdirent.parent_attr;
    s_op->val.buffer_sz = sizeof(PVFS_object_attr);

    gossip_debug(SERVER_DEBUG,
		 "  reading metadata (coll_id = 0x%x, handle = 0x%08Lx, key = %s (%d), val_buf = 0x%08x (%d))\n",
		 s_op->req->u.crdirent.fs_id,
		 s_op->req->u.crdirent.parent_handle,
		 (char *) s_op->key.buffer,
		 s_op->key.buffer_sz,
		 (unsigned) s_op->val.buffer,
		 s_op->val.buffer_sz);

    job_post_ret = job_trove_keyval_read(s_op->req->u.crdirent.fs_id,
					 s_op->req->u.crdirent.parent_handle,
					 &s_op->key,
					 &s_op->val,
					 0,
					 NULL,
					 s_op,
					 ret,
					 &i);
    return job_post_ret;
}

/*
 * Function: crdirent_verify_parent_metadata
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      s_op->val.buffer is a valid PVFS_object_attr structure
 *
 * Post:     User has permission to perform operation
 *
 * Returns:  int
 *
 * Synopsis: This should use a global function that verifies that the user
 *           has the necessary permissions to perform the operation it wants
 *           to do.
 *           
 */
static int crdirent_verify_parent_metadata(PINT_server_op *s_op,
					   job_status_s *ret)
{
    PVFS_object_attr *a_p;

    a_p = &s_op->u.crdirent.parent_attr;

    gossip_debug(SERVER_DEBUG,"crdirent state: verify_parent_metadata\n");

    gossip_debug(SERVER_DEBUG,
		 "  attrs = (owner = %d, group = %d, perms = %o, type = %d)\n",
		 a_p->owner,
		 a_p->group,
		 a_p->perms,
		 a_p->objtype);

    /* recall attributes are in s_op->u.crdirent.parent_attr */
    /* TODO: MAKE THIS AN ERROR CHECK INSTEAD LATER */
    assert(s_op->u.crdirent.parent_attr.objtype == PVFS_TYPE_DIRECTORY);

    /*IF THEY don't have permission, set ret->error_code to -ENOPERM!*/

    return 1;
}


/*
 * Function: crdirent_read_directory_entry_handle
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      s_op->u.crdirent.parent_handle is handle of directory
 *
 * Post:     s_op->val.buffer is the directory entry k/v space OR NULL
 *           if first entry
 *
 * Returns:  int
 *
 * Synopsis: Given a directory handle, look up the handle used to store
 * directory entries for this directory.
 *
 *           Get the directory entry handle for the directory entry k/v space.
 *           Recall that directories have two key-val spaces, one of which is 
 *           synonymous with files where the metadata is stored.  The other
 *           space holds the filenames and their handles.  In this function, 
 *           we attempt to retrieve the handle for the filename/handle key/val
 *           space and if it does not exist, we need to create it.
 *
 *           TODO: Semantics here of whether we want to create it here, or upon
 *                 the creation of the directory. 
 *           
 */
static int crdirent_read_directory_entry_handle(PINT_server_op *s_op,
						job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: gethandle\n");

    /* get the key and key size out of our list of common keys */
    s_op->key.buffer    = Trove_Common_Keys[DIR_ENT_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[DIR_ENT_KEY].size;

    /* store the handle in the crdirent-specific space in s_op */
    s_op->val.buffer    = &s_op->u.crdirent.dirent_handle;
    s_op->val.buffer_sz = sizeof(PVFS_handle);

    job_post_ret = job_trove_keyval_read(s_op->req->u.crdirent.fs_id,
					 s_op->req->u.crdirent.parent_handle,
					 &s_op->key,
					 &s_op->val,
					 0,
					 NULL,
					 s_op,
					 ret,
					 &i);
    return job_post_ret;
}

/*
 * Function: crdirent_create_dirdata_dspace
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      ret->handle = NULL
 *           ret->error_code < 0
 *
 * Post:     ret->handle contains a new handle for a k/v space
 *
 * Returns:  int
 *
 * Synopsis: If we execute this function, this directory does not have
 *           any entries in it.  So we need to create a key val space
 *           for these entries.  This is the first part, and we store
 *           it in part two.
 */
static int crdirent_create_dirdata_dspace(PINT_server_op *s_op,
					  job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: create_dirdata_dspace\n");

    gossip_debug(SERVER_DEBUG, "  creating dspace (coll_id = 0x%x)\n",
		 s_op->req->u.crdirent.fs_id);

    job_post_ret = job_trove_dspace_create(s_op->req->u.crdirent.fs_id,
					   0, /* TODO: WHAT SHOULD HANDLE BE? */
					   0x00000000, /* TODO: Change this */
					   PVFS_TYPE_DIRDATA,
					   NULL,
					   s_op,
					   ret,
					   &i);
    return job_post_ret;
}

/*
 * Function: crdirent_write_dirdata_handle
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      ret->handle is the new directory entry k/v space
 *
 * Post:     ret->handle is stored in the original k/v space for the
 *           parent handle.
 *
 * Returns:  int
 *
 * Synopsis: We are storing the newly created k/v space for future
 *           directory entries.
 *           
 */
static int crdirent_write_dirdata_handle(PINT_server_op *s_op,
					 job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: write_dirdata_handle\n");

    /* TODO CHECK ERROR CODE */

    /* get the key and key size out of our list of common keys */
    s_op->key.buffer    = Trove_Common_Keys[DIR_ENT_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[DIR_ENT_KEY].size;

    /* the last state was create_dirdata_dspace, so handle is in ret->handle. save it. */
    s_op->u.crdirent.dirent_handle = ret->handle;

    /* get the handle out of the crdirent scratch space */
    s_op->val.buffer    = &s_op->u.crdirent.dirent_handle;
    s_op->val.buffer_sz = sizeof(PVFS_handle);

    gossip_debug(SERVER_DEBUG,
		 "  writing dirdata handle (0x%08Lx) into parent directory keyval space (0x%08Lx)\n",
		 s_op->u.crdirent.dirent_handle,
		 s_op->req->u.crdirent.parent_handle);

    job_post_ret = job_trove_keyval_write(s_op->req->u.crdirent.fs_id,
					  s_op->req->u.crdirent.parent_handle,
					  &s_op->key,
					  &s_op->val,
					  TROVE_SYNC,
					  NULL,
					  s_op,
					  ret,
					  &i);
    return job_post_ret;
}


/*
 * Function: crdirent_write_directory_entry
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      ret->handle is the directory entry k/v space
 *           s_op->req->u.crdirent.name != NULL
 *           s_op->req->u.crdirent.new_handle != NULL
 *           ADD ASSERTS FOR THESE!
 *
 * Post:     key/val pair stored
 *
 * Returns:  int
 *
 * Synopsis: We are now ready to store the name/handle pair in the k/v
 *           space for directory handles.
 */

static int crdirent_write_directory_entry(PINT_server_op *s_op,
					  job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: write_directory_entry\n");

    /* This buffer came from one of two places, either phase two of creating the
     * directory space when we wrote the value back to trove, or from the initial read
     * from trove.
     */

    /* this is the name for the parent entry */
    s_op->key.buffer    = s_op->req->u.crdirent.name;
    s_op->key.buffer_sz = strlen(s_op->req->u.crdirent.name) + 1;

    s_op->val.buffer    = &s_op->req->u.crdirent.new_handle;
    s_op->val.buffer_sz = sizeof(PVFS_handle);

    gossip_debug(SERVER_DEBUG,
		 "  writing new directory entry for %s (handle = 0x%08Lx) to dirdata dspace 0x%08Lx\n",
		 s_op->req->u.crdirent.name,
		 s_op->req->u.crdirent.new_handle,
		 s_op->u.crdirent.dirent_handle);

    job_post_ret = job_trove_keyval_write(s_op->req->u.crdirent.fs_id,
					  s_op->u.crdirent.dirent_handle,
					  &s_op->key,
					  &s_op->val,
					  TROVE_SYNC,
					  NULL,
					  s_op,
					  ret,
					  &i);
    return job_post_ret;
}

/*
 * Function: crdirent_bmi_send
 *
 * Params:   server_op *s_op, 
 *           job_status_s *ret
 *
 * Pre:      NONE
 *
 * Post:     BMI_message sent.
 *
 * Returns:  int
 *
 * Synopsis: We sent a response to the request using BMI.
 *           This function is abstract because we really don't know where
 *           we failed or if we succeeded in our mission.  It sets the
 *           error_code, and here, it is just an acknowledgement.
 */

static int crdirent_send_bmi(PINT_server_op *s_op,
			     job_status_s *ret)
{
    int job_post_ret=0;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: send_bmi\n");

    s_op->resp->status = ret->error_code;
    s_op->resp->rsize = sizeof(struct PVFS_server_resp_s);

    if(ret->error_code == 0) 
    {
	s_op->resp->u.generic.handle = s_op->req->u.crdirent.new_handle;
    }

    /* Encode the message */
    job_post_ret = PINT_encode(s_op->resp,
			       PINT_ENCODE_RESP,
			       &(s_op->encoded),
			       s_op->addr,
			       s_op->enc_type);

    assert(job_post_ret == 0);

    job_post_ret = job_bmi_send_list(s_op->addr,
				     s_op->encoded.buffer_list,
				     s_op->encoded.size_list,
				     s_op->encoded.list_count,
				     s_op->encoded.total_size,
				     s_op->tag,
				     s_op->encoded.buffer_flag,
				     0,
				     s_op, 
				     ret, 
				     &i);
    return job_post_ret;
}

/*
 * Function: crdirent_release_posted_job
 *
 * Params:   server_op *b, 
 *           job_status_s *ret
 *
 * Pre:      We are done!
 *
 * Post:     We need to let the next operation go.
 *
 * Returns:  int
 *
 * Synopsis: Free the job from the scheduler to allow next job to proceed.
 */

static int crdirent_release_posted_job(PINT_server_op *s_op,
				       job_status_s *ret)
{

    int job_post_ret=0;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "crdirent state: release_posted_job\n");

    if (s_op->scheduled_id == 0) {
	gossip_debug(SERVER_DEBUG, "  skipping; didn't schedule this request\n");
	return 1;
    }
    else {
	job_post_ret = job_req_sched_release(s_op->scheduled_id,
					     s_op,
					     ret,
					     &i);
	return job_post_ret;
    }
}

/*
 * Function: crdirent_cleanup
 *
 * Synopsis: free memory and return
 */

static int crdirent_cleanup(PINT_server_op *s_op,
			    job_status_s *ret)
{
    gossip_debug(SERVER_DEBUG, "crdirent state: crdirent_cleanup\n");

    /* free decoded, encoded requests */
    PINT_decode_release(&s_op->decoded, PINT_DECODE_REQ, 0);
    free(s_op->unexp_bmi_buff.buffer);

    /* free encoded, decoded responses */
    PINT_encode_release(&s_op->encoded, PINT_ENCODE_RESP, 0);
    free(s_op->resp);

    /* free server operation structure */
    free(s_op);

    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */


