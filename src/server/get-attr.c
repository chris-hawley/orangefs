/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */


/*

SMS:  1. Errors should be handled correctly
      2. Request Scheduler used
      3. Documented
					      
SFS:  1. Needs some pre/post
      2. Some assertions
		      
TS:   Implemented but not thorough.

My TODO list for this SM:

 Finish asserts and documentation.  Again, this is a fairly trivial machine

*/

#include <string.h>
#include <assert.h>

#include "state-machine.h"
#include "server-config.h"
#include "pvfs2-server.h"
#include "pvfs2-attr.h"
#include "job-consist.h"

enum {
    STATE_METAFILE = 7,
};


static int getattr_init(state_action_struct *s_op, job_status_s *ret);
static int getattr_cleanup(state_action_struct *s_op, job_status_s *ret);
static int getattr_getobj_attribs(state_action_struct *s_op, job_status_s *ret);
static int getattr_release_posted_job(state_action_struct *s_op, job_status_s *ret);
static int getattr_send_bmi(state_action_struct *s_op, job_status_s *ret);
static int getattr_verify_attribs(state_action_struct *s_op, job_status_s *ret);
static int getattr_read_metafile_datafile_handles(state_action_struct *s_op, job_status_s *ret);
static int getattr_read_metafile_distribution(state_action_struct *s_op, job_status_s *ret);
void getattr_init_state_machine(void);

extern PINT_server_trove_keys_s Trove_Common_Keys[];

PINT_state_machine_s getattr_req_s = 
{
	NULL,
	"getattr",
	getattr_init_state_machine
};

%%

machine get_attr(init,
		 cleanup,
		 getobj_attrib,
		 send_bmi,
		 release,
		 verify_attribs,
		 read_metafile_datafile_handles,
		 read_metafile_distribution)
{
	state init
	{
		run getattr_init;
		default => getobj_attrib;
	}

	state getobj_attrib
	{
		run getattr_getobj_attribs;
		default => verify_attribs;
	}

	state verify_attribs
	    {
		run getattr_verify_attribs;
		STATE_METAFILE => read_metafile_datafile_handles;
		default => send_bmi;
	    }

	state read_metafile_datafile_handles
	    {
		run getattr_read_metafile_datafile_handles;
		default => read_metafile_distribution;
	    }
	state read_metafile_distribution
	    {
		run getattr_read_metafile_distribution;
		default => send_bmi;
	    }

	state send_bmi
	{
		run getattr_send_bmi;
		default => release;
	}

	state release
	{
		run getattr_release_posted_job;
		default => cleanup;
	}

	state cleanup
	{
		run getattr_cleanup;
		default => init;
	}
}

%%

/*
 * Function: getattr_init_state_machine
 *
 * Params:   void
 *
 * Returns:  PINT_state_array_values*
 *
 * Synopsis: Set up the state machine for get_attrib. 
 *           
 */


void getattr_init_state_machine(void)
{

    getattr_req_s.state_machine = get_attr;

}

/*
 * Function: getattr_init
 *
 * Params:   server_op *b, 
 *           job_status_s *ret
 *
 * Returns:  int
 *
 * Synopsis: We will need to allocate a buffer large enough to store the 
 *           attributes the client is requesting.  Also, schedule it for
 *           consistency semantics.
 *           
 */


static int getattr_init(state_action_struct *s_op, job_status_s *ret)
{

    int job_post_ret;

    gossip_debug(SERVER_DEBUG, "getattr state: init\n");

    s_op->resp->op = s_op->req->op;

    job_post_ret = job_req_sched_post(s_op->req,
				      s_op,
				      ret,
				      &(s_op->scheduled_id));

    return(job_post_ret);

}


/*
 * Function: getattr_getobj_attrib
 *
 * Params:   server_op *b, 
 *           job_status_s *ret
 *
 * Returns:  int
 *
 * Synopsis: Post a trove operation to fetch the attributes
 *           
 *           
 */

static int getattr_getobj_attribs(state_action_struct *s_op, job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "getattr state: getobj_attribs\n");

    s_op->key.buffer    = Trove_Common_Keys[METADATA_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[METADATA_KEY].size;

    /* TODO: READ INTO A TEMPORARY BUFFER AND THEN EXTRACT JUST WHAT THEY WANT */
    s_op->val.buffer    = &s_op->resp->u.getattr.attr;
    s_op->val.buffer_sz = sizeof(PVFS_object_attr);

    gossip_debug(SERVER_DEBUG,
		 "  reading attributes (coll_id = 0x%x, handle = 0x%08Lx, key = %s (%d), val_buf = 0x%08x (%d))\n",
		 s_op->req->u.getattr.fs_id,
		 s_op->req->u.getattr.handle,
		 (char *) s_op->key.buffer,
		 s_op->key.buffer_sz,
		 (unsigned) s_op->val.buffer,
		 s_op->val.buffer_sz);

    job_post_ret = job_trove_keyval_read(s_op->req->u.getattr.fs_id,
					 s_op->req->u.getattr.handle,
					 &(s_op->key),
					 &(s_op->val),
					 0,
					 NULL,
					 s_op,
					 ret,
					 &i);
    return(job_post_ret);
}

/* getattr_verify_attribs
 */
static int getattr_verify_attribs(state_action_struct *s_op, job_status_s *ret)
{
    PVFS_object_attr *a_p;

    gossip_debug(SERVER_DEBUG, "getattr state: verify_attribs\n");

    /* attributes were read into s_op->val.buffer in previous state,
     * which was pointing to s_op->resp->u.getattr.attr;
     */
    a_p = (PVFS_object_attr *) s_op->val.buffer;
    a_p = &s_op->resp->u.getattr.attr;

    if (ret->error_code != 0) {
	gossip_debug(SERVER_DEBUG,
		     "  previous keyval read had an error (new metafile?); data is useless\n");
    }
    else {
	gossip_debug(SERVER_DEBUG,
		     "  attrs read from dspace = (owner = %d, group = %d, perms = %o, type = %d)\n",
		     a_p->owner,
		     a_p->group,
		     a_p->perms,
		     a_p->objtype);
    }

    /* TODO: HANDLE TYPES OTHER THAN METAFILES TOO, SOME DAY... */
    if ((ret->error_code != 0 && s_op->resp->u.getattr.attr.objtype == PVFS_TYPE_METAFILE) || a_p->objtype == PVFS_TYPE_METAFILE)
    {
	gossip_debug(SERVER_DEBUG,
		     "  handle 0x%08Lx refers to a metafile\n",
		     s_op->req->u.getattr.handle);
	ret->error_code = STATE_METAFILE;

	/* save # of datafiles so we know the size for reading later */
	s_op->resp->u.getattr.attr.u.meta.nr_datafiles = a_p->u.meta.nr_datafiles;
	/* TODO: keep distribution size in here too */
    }
    else if (ret->error_code == 0) {
	gossip_debug(SERVER_DEBUG,
		     "  handle 0x%08Lx refers to something other than a metafile\n",
		     s_op->req->u.getattr.handle);
    }
    else {
	/* got an error reading */
	gossip_debug(SERVER_DEBUG,
		     "  error reading attributes for handle 0x%08Lx; sending response\n",
		     s_op->req->u.getattr.handle);
    }
    
    return 1;
}

/* getattr_read_metafile_datafile_handles
 */
static int getattr_read_metafile_datafile_handles(state_action_struct *s_op, job_status_s *ret)
{
    int nr_datafiles;
    int job_post_ret;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "getattr state: read_metafile_datafile_handles\n");

    nr_datafiles = s_op->resp->u.getattr.attr.u.meta.nr_datafiles;

    s_op->key.buffer    = Trove_Common_Keys[METAFILE_HANDLES_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[METAFILE_HANDLES_KEY].size;

    /* should be more than one datafile. */
    assert(nr_datafiles > 0);

    /* NOTE: ALLOCATING MEMORY HERE */
    s_op->resp->u.getattr.attr.u.meta.dfh = malloc(nr_datafiles * sizeof(PVFS_handle));
    if(!s_op->resp->u.getattr.attr.u.meta.dfh)
    {
	ret->error_code = -errno;
	return(1);
    }

    s_op->val.buffer    = s_op->resp->u.getattr.attr.u.meta.dfh;
    s_op->val.buffer_sz = nr_datafiles * sizeof(PVFS_handle);

    gossip_debug(SERVER_DEBUG,
		 "  reading %d datafile handles (coll_id = 0x%x, handle = 0x%08Lx, key = %s (%d), val_buf = 0x%08x (%d))\n",
		 nr_datafiles,
		 s_op->req->u.getattr.fs_id,
		 s_op->req->u.getattr.handle,
		 (char *) s_op->key.buffer,
		 s_op->key.buffer_sz,
		 (unsigned) s_op->val.buffer,
		 s_op->val.buffer_sz);

    job_post_ret = job_trove_keyval_read(s_op->req->u.getattr.fs_id,
					 s_op->req->u.getattr.handle,
					 &s_op->key,
					 &s_op->val,
					 0,
					 NULL,
					 s_op,
					 ret,
					 &i);

    return job_post_ret;
}

/* getattr_read_metafile_distribution
 */
static int getattr_read_metafile_distribution(state_action_struct *s_op, job_status_s *ret)
{
    int job_post_ret;
    job_id_t i;

    s_op->key.buffer    = Trove_Common_Keys[METAFILE_DIST_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[METAFILE_DIST_KEY].size;

    /* should be some distribution information. */
    assert(s_op->resp->u.getattr.attr.u.meta.dist_size > 0);

    /* NOTE: allocating memory here, free'd in last state */
    s_op->resp->u.getattr.attr.u.meta.dist =
	malloc(s_op->resp->u.getattr.attr.u.meta.dist_size);
    if(!s_op->resp->u.getattr.attr.u.meta.dist)
    {
	ret->error_code = -errno;
	return(1);
    }

    s_op->val.buffer    = s_op->resp->u.getattr.attr.u.meta.dist;
    s_op->val.buffer_sz = s_op->resp->u.getattr.attr.u.meta.dist_size;

    job_post_ret = job_trove_keyval_read(s_op->req->u.getattr.fs_id,
					 s_op->req->u.getattr.handle,
					 &(s_op->key),
					 &(s_op->val),
					 0,
					 NULL,
					 s_op,
					 ret,
					 &i);
    return job_post_ret;
}

/*
 * Function: getattr_send_bmi
 *
 * Params:   server_op *b, 
 *           job_status_s *ret
 *
 * Returns:  int
 *
 * Synopsis: Send the message and resulting data to the client.
 *           
 */

static int getattr_send_bmi(state_action_struct *s_op, job_status_s *ret)
{
    PVFS_object_attr *a_p;
    int job_post_ret=0;
    job_id_t i;

    gossip_debug(SERVER_DEBUG, "getattr state: send_bmi\n");

    a_p = &s_op->resp->u.getattr.attr;
    
    /* TODO: figure out if we need a different packing mechanism here */
    gossip_err("KLUDGE: reading distribution on disk in network encoded format.\n");
    PINT_Dist_decode(a_p->u.meta.dist, NULL);    


    gossip_debug(SERVER_DEBUG,
		 "  sending attrs (owner = %d, group = %d, perms = %o, type = %d)\n",
		 a_p->owner,
		 a_p->group,
		 a_p->perms,
		 a_p->objtype);

    /* Prepare the message */
    s_op->resp->status = ret->error_code;

    s_op->resp->rsize = sizeof(struct PVFS_server_resp_s);
    if (a_p->objtype == PVFS_TYPE_METAFILE) {
	gossip_debug(SERVER_DEBUG,
		     "  also returning %d datafile handles\n",
		     a_p->u.meta.nr_datafiles);

	s_op->resp->rsize += (a_p->u.meta.nr_datafiles *
	    sizeof(PVFS_handle)) + a_p->u.meta.dist_size;
    }

    gossip_debug(SERVER_DEBUG,
		 "  sending status %d, rsize = %Ld\n",
		 s_op->resp->status,
		 s_op->resp->rsize);

    job_post_ret = PINT_encode(s_op->resp,
			       PINT_ENCODE_RESP,
			       &(s_op->encoded),
			       s_op->addr,
			       s_op->enc_type);

    assert(job_post_ret == 0);

    /* Post message */
#ifndef PVFS2_SERVER_DEBUG_BMI

    job_post_ret = job_bmi_send_list(
				     s_op->addr,
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

#else

    job_post_ret = job_bmi_send(
				s_op->addr,
				s_op->encoded.buffer_list[0],
				s_op->encoded.total_size,
				s_op->tag,
				s_op->encoded.buffer_flag,
				0,
				s_op,
				ret,
				&i);

#endif

    return(job_post_ret);

}


/*
 * Function: getattr_release_posted_job
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

static int getattr_release_posted_job(state_action_struct *s_op, job_status_s *ret)
{

    int job_post_ret=0;
    job_id_t i;

    job_post_ret = job_req_sched_release(s_op->scheduled_id,
	    s_op,
	    ret,
	    &i);
    return job_post_ret;
}


/*
 * Function: getattr_cleanup
 *
 * Params:   server_op *b, 
 *           job_status_s *ret
 *
 * Pre:      Memory has been allocated
 *
 * Post:     All Allocated memory has been freed.
 *
 * Returns:  int
 *
 * Synopsis: free memory and return
 *           
 */


static int getattr_cleanup(state_action_struct *s_op, job_status_s *ret)
{

    gossip_debug(SERVER_DEBUG, "getattr state: cleanup\n");

    /* free decoded, encoded requests */
    PINT_decode_release(&(s_op->decoded),PINT_DECODE_REQ,0);
    free(s_op->unexp_bmi_buff.buffer);

    /* free encoded response */
    PINT_encode_release(&(s_op->encoded),PINT_ENCODE_RESP,0);

    /* if this is a metafile, we allocated space for the datafile handles; free that. */
    if (s_op->resp->u.getattr.attr.objtype == PVFS_TYPE_METAFILE) {
	free(s_op->resp->u.getattr.attr.u.meta.dfh);
	free(s_op->resp->u.getattr.attr.u.meta.dist);
    }

    /* free response */
    free(s_op->resp);

    /* free server operation structure */
    free(s_op);

    return(0);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */

