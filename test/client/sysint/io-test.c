/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#include <client.h>
#include <sys/time.h>
#include <unistd.h>

extern int parse_pvfstab(char *fn,pvfs_mntlist *mnt);

int main(int argc,char **argv)
{
	PVFS_sysresp_init resp_init;
	PVFS_sysreq_lookup req_lk;
	PVFS_sysresp_lookup resp_lk;
	PVFS_sysreq_create req_cr;
	PVFS_sysresp_create resp_cr;
	PVFS_sysreq_io req_io;
	PVFS_sysresp_io resp_io;
	char *filename;
	int name_sz;
	int ret = -1;
	pvfs_mntlist mnt = {0,NULL};
	int io_size = 1024 * 1024;
	int* io_buffer = NULL;
	int i;
	int errors;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <file name>\n", argv[0]);
		return(-1);
	}

	if(index(argv[1], '/'))
	{
		fprintf(stderr, "Error: please use simple file names, no path.\n");
		return(-1);
	}
		
	/* create a buffer for running I/O on */
	io_buffer = (int*)malloc(io_size*sizeof(int));
	if(!io_buffer)
	{
		return(-1);
	}

	/* put some data in the buffer so we can verify */
	for (i=0; i<io_size; i++) {
		io_buffer[i]=i;
	}

	/* build the full path name (work out of the root dir for this test) */

	name_sz = strlen(argv[1]) + 2; /* include null terminator and slash */
	filename = malloc(name_sz);
	if(!filename)
	{
		perror("malloc");
		return(-1);
	}
	filename[0] = '/';

	memcpy(&(filename[1]), argv[1], (name_sz-1));

	/* parse pvfstab */
	ret = parse_pvfstab(NULL,&mnt);
	if (ret < 0)
	{
		fprintf(stderr, "Error: parse_pvfstab() failure.\n");
		return(-1);
	}
	/* init the system interface */
	ret = PVFS_sys_initialize(mnt, &resp_init);
	if(ret < 0)
	{
		fprintf(stderr, "Error: PVFS_sys_initialize() failure. = %d\n", ret);
		return(ret);
	}

	/*************************************************************
	 * try to look up the target file 
	 */
	
	req_lk.name = filename;
	req_lk.fs_id = resp_init.fsid_list[0];
	req_lk.credentials.uid = 100;
	req_lk.credentials.gid = 100;
	req_lk.credentials.perms = U_WRITE|U_READ;

	ret = PVFS_sys_lookup(&req_lk, &resp_lk);
	/* TODO: really we probably want to look for a specific error code,
	 * like maybe ENOENT?
	 */
	if (ret < 0)
	{
		printf("IO-TEST: lookup failed; creating new file.\n");

		/* get root handle */
		req_lk.name = "/";
		req_lk.fs_id = resp_init.fsid_list[0];
		req_lk.credentials.uid = 100;
		req_lk.credentials.gid = 100;
		req_lk.credentials.perms = U_WRITE|U_READ;

		ret = PVFS_sys_lookup(&req_lk, &resp_lk);
		if(ret < 0)
		{
			fprintf(stderr, "Error: PVFS_sys_lookup() failed to find root handle.\n");
			return(-1);
		}

		/* create new file */

		/* TODO: I'm not setting the attribute mask... not real sure
		 * what's supposed to happen there */
		req_cr.attr.owner = 100;
		req_cr.attr.group = 100;
		req_cr.attr.perms = U_WRITE|U_READ;
		req_cr.attr.u.meta.nr_datafiles = 1;
		req_cr.attr.u.meta.dist = NULL;
		req_cr.parent_refn.handle = resp_lk.pinode_refn.handle;
		req_cr.parent_refn.fs_id = req_lk.fs_id;
		req_cr.entry_name = &(filename[1]); /* leave off slash */
		req_cr.credentials.uid = 100;
		req_cr.credentials.gid = 100;
		req_cr.credentials.perms = U_WRITE|U_READ;

		ret = PVFS_sys_create(&req_cr, &resp_cr);
		if(ret < 0)
		{
			fprintf(stderr, "Error: PVFS_sys_create() failure.\n");
			return(-1);
		}

		req_io.pinode_refn.fs_id = req_lk.fs_id;
		req_io.pinode_refn.handle = resp_cr.pinode_refn.handle;
	}
	else
	{
		printf("IO-TEST: lookup succeeded; performing I/O on existing file.\n");

		req_io.pinode_refn.fs_id = req_lk.fs_id;
		req_io.pinode_refn.handle = resp_lk.pinode_refn.handle;
	}

	/**************************************************************
	 * carry out I/O operation
	 */

	printf("IO-TEST: performing write on handle: %ld, fs: %d\n",
		(long)req_io.pinode_refn.handle, (int)req_io.pinode_refn.fs_id);

	req_io.credentials.uid = 100;
	req_io.credentials.gid = 100;
	req_io.credentials.perms = U_WRITE|U_READ;
	req_io.buffer = io_buffer;
	req_io.buffer_size = io_size*sizeof(int);

	ret = PVFS_Request_contiguous(io_size*sizeof(int), PVFS_BYTE, &(req_io.io_req));
	if(ret < 0)
	{
		fprintf(stderr, "Error: PVFS_Request_contiguous() failure.\n");
		return(-1);
	}

	ret = PVFS_sys_write(&req_io, &resp_io);
	if(ret < 0)
	{
		fprintf(stderr, "Error: PVFS_sys_write() failure.\n");
		return(-1);
	}

	printf("IO-TEST: wrote %d bytes.\n", (int)resp_io.total_completed);

	/* uncomment and try out the readback-and-verify stuff that follows
	 * once reading back actually works */
	memset(io_buffer, 0, io_size*sizeof(int));

	/* verify */
	printf("IO-TEST: performing read on handle: %ld, fs: %d\n",
		(long)req_io.pinode_refn.handle, (int)req_io.pinode_refn.fs_id);
	ret = PVFS_sys_read(&req_io, &resp_io);
	if(ret < 0)
	{
		fprintf(stderr, "Error: PVFS_sys_read() failure.\n");
		return(-1);
	}
	printf("IO-TEST: read %d bytes.\n", (int)resp_io.total_completed);
	if((io_size*sizeof(int)) != resp_io.total_completed)
	{
		fprintf(stderr, "Error: SHORT READ! skipping verification...\n");
	}
	else
	{
		errors = 0;
		for(i=0; i<io_size; i++) {
			if (i != io_buffer[i] )
			{
				fprintf(stderr, "error: element %d differs: should be %d, is %d\n", i, i, io_buffer[i]); 
				errors++;
			}
		}
		if (errors != 0 )
		{
			fprintf(stderr, "ERROR: found %d errors\n", errors);
		}
		else
		{
			printf("IO-TEST: no errors found.\n");
		}
	}

	/**************************************************************
	 * shut down pending interfaces
	 */

	ret = PVFS_sys_finalize();
	if (ret < 0)
	{
		fprintf(stderr, "Error: PVFS_sys_finalize() failed with errcode = %d\n", ret);
		return (-1);
	}

	free(filename);
	free(io_buffer);
	return(0);
}
