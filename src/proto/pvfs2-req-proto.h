/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#ifndef __PVFS2_REQ_PROTO_H
#define __PVFS2_REQ_PROTO_H

#include "pvfs2-config.h"
#include "pvfs2-types.h"
#include "pvfs2-attr.h"
#include "pvfs-distribution.h"
#include "pvfs2-request.h"
#include "pvfs2-mgmt.h"

/* release number:
 * This is a base-10, 5 digit number, with one digit for the most
 * significant version number and two for the last two (e.g. 1.5.6 => 10506)
 */
#define PVFS_RELEASE_NR (PVFS2_VERSION_MAJOR*10000\
 + PVFS2_VERSION_MINOR*100 + PVFS2_VERSION_SUB)

/* enumeration of supported server operations */
enum PVFS_server_op
{
    PVFS_SERV_INVALID = 0,
    PVFS_SERV_CREATE = 1,
    PVFS_SERV_REMOVE = 2,
    PVFS_SERV_IO = 3,
    PVFS_SERV_GETATTR = 4,
    PVFS_SERV_SETATTR = 5,
    PVFS_SERV_LOOKUP_PATH = 6,
    PVFS_SERV_CRDIRENT = 7,
    PVFS_SERV_RMDIRENT = 8,
    PVFS_SERV_TRUNCATE = 9,
    PVFS_SERV_MKDIR = 10,
    PVFS_SERV_READDIR = 11,
    PVFS_SERV_GETCONFIG = 12,
    PVFS_SERV_WRITE_COMPLETION = 13,
    PVFS_SERV_FLUSH = 14,
    PVFS_SERV_MGMT_SETPARAM = 15,
    PVFS_SERV_MGMT_NOOP = 16,
    PVFS_SERV_STATFS = 17,
    PVFS_SERV_PERF_UPDATE = 18,  /* not a real protocol request */
    PVFS_SERV_MGMT_PERF_MON = 19,
    PVFS_SERV_MGMT_ITERATE_HANDLES = 20,
    PVFS_SERV_MGMT_DSPACE_INFO_LIST = 21,
    PVFS_SERV_MGMT_EVENT_MON = 22
    /* IMPORTANT: please remember to modify PVFS_MAX_SERVER_OP define (below)
     * if you add a new operation to this list
     */

    /* not implemented:
     * PVFS_SERV_GETEATTR
     * PVFS_SERV_SETEATTR,
     * PVFS_SERV_BATCH
     * PVFS_SERV_EXTENSION
     */
};
#define PVFS_MAX_SERVER_OP 22

/******************************************************************/
/* these values define limits on the maximum size of variable length
 * parameters used within the request protocol
 */

/* max size of opaque distribution parameters */
#define PVFS_REQ_LIMIT_DIST_BYTES         1024
/* max size of each configuration file transmitted to clients */
#define PVFS_REQ_LIMIT_CONFIG_FILE_BYTES  16384
/* max size of all path strings */
#define PVFS_REQ_LIMIT_PATH_NAME_BYTES    PVFS_NAME_MAX
/* max size of strings representing a single path element */
#define PVFS_REQ_LIMIT_SEGMENT_BYTES      PVFS_SEGMENT_MAX
/* max total size of I/O request descriptions */
#define PVFS_REQ_LIMIT_IOREQ_BYTES        8192
/* max count of segments allowed per path lookup (note that this governs 
 * the number of handles and attributes returned in lookup_path responses)
 */
#define PVFS_REQ_LIMIT_PATH_SEGMENT_COUNT 256
/* max count of datafiles associated with a logical file */
#define PVFS_REQ_LIMIT_DFILE_COUNT        1024
/* max count of directory entries per readdir request */
#define PVFS_REQ_LIMIT_DIRENT_COUNT       64
/* max number of perf metrics returned by mgmt perf mon op */
#define PVFS_REQ_LIMIT_MGMT_PERF_MON_COUNT 16
/* max number of events returned by mgmt event mon op */
#define PVFS_REQ_LIMIT_MGMT_EVENT_MON_COUNT 2048
/* max number of handles returned by any operation using an array of handles */
#define PVFS_REQ_LIMIT_HANDLES_COUNT 1024
/* max number of handles returned by mgmt iterate handles op */
#define PVFS_REQ_LIMIT_MGMT_ITERATE_HANDLES_COUNT PVFS_REQ_LIMIT_HANDLES_COUNT
/* max number of info list items returned by mgmt dspace info list op */
/* max number of dspace info structs returned by mgmt dpsace info op */
#define PVFS_REQ_LIMIT_MGMT_DSPACE_INFO_LIST_COUNT 1024
/* max number of path elements in a lookup_attr response */
#define PVFS_REQ_LIMIT_MAX_PATH_ELEMENTS  40

/* create *********************************************************/
/* - used to create new metafile and datafile objects */

struct PVFS_servreq_create
{
    PVFS_fs_id fs_id;
    PVFS_ds_type object_type;	    /* type of object to create */

    /*
      an array of handle extents that we use to suggest to
      the server from which handle range to allocate for the
      newly created handle(s).  To request a single handle,
      a single extent with first = last should be used.
    */
    PVFS_handle_extent_array handle_extent_array;
};
endecode_fields_3_struct(PVFS_servreq_create,
    PVFS_fs_id, fs_id,
    PVFS_ds_type, object_type,
    PVFS_handle_extent_array, handle_extent_array);
#define extra_size_PVFS_servreq_create \
    (PVFS_REQ_LIMIT_HANDLES_COUNT * sizeof(PVFS_handle_extent))

#define PINT_SERVREQ_CREATE_FILL(__req,                \
                                 __creds,              \
                                 __fsid,               \
                                 __objtype,            \
                                 __ext_array)          \
do {                                                   \
    memset(&(__req), 0, sizeof(__req));                \
    (__req).op = PVFS_SERV_CREATE;                     \
    (__req).credentials = (__creds);                   \
    (__req).u.create.fs_id = (__fsid);                 \
    (__req).u.create.object_type = (__objtype);        \
    (__req).u.create.handle_extent_array.extent_count =\
        (__ext_array.extent_count);                    \
    (__req).u.create.handle_extent_array.extent_array =\
        (__ext_array.extent_array);                    \
} while (0)

struct PVFS_servresp_create
{
    PVFS_handle handle;
};
endecode_fields_1_struct(PVFS_servresp_create, PVFS_handle, handle);

/* remove *****************************************************/
/* - used to remove an existing metafile or datafile object */

struct PVFS_servreq_remove
{
    PVFS_handle handle;		    /* handle of object to remove */
    PVFS_fs_id fs_id;		    /* file system */
};
endecode_fields_2_struct(PVFS_servreq_remove,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id);

#define PINT_SERVREQ_REMOVE_FILL(__req,		\
				 __creds,	\
				 __fsid,	\
				 __handle)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_REMOVE;		\
    (__req).credentials = (__creds);		\
    (__req).u.remove.fs_id = (__fsid);		\
    (__req).u.remove.handle = (__handle);	\
} while (0)

/* NOTE: no response structure; all necessary response info is 
 * returned in generic server response structure
 */

/* flush
 * - used to flush an object to disk */
struct PVFS_servreq_flush
{
    PVFS_handle handle;		/* handle of object to flush */
    PVFS_fs_id fs_id;		/* file system */
    int flags;			/* future use */
};
endecode_fields_3_struct(PVFS_servreq_flush,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id,
  int32_t, flags);

#define PINT_SERVREQ_FLUSH_FILL(__req,		\
				__creds,	\
				__fsid,		\
				__handle)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_FLUSH;		\
    (__req).credentials = (__creds);		\
    (__req).u.flush.fs_id = (__fsid);		\
    (__req).u.flush.handle = (__handle);	\
} while (0)

/* NOTE: no response structure; all necessary response info is returned in
 * generic server response structure
 */

/* getattr ****************************************************/
/* - retreives attributes based on mask of PVFS_ATTR_XXX values */

struct PVFS_servreq_getattr
{
    PVFS_handle handle;		    /* handle of target object */
    PVFS_fs_id fs_id;		    /* file system */
    uint32_t attrmask;		    /* mask of desired attributes */
};
endecode_fields_3_struct(PVFS_servreq_getattr,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id,
  uint32_t, attrmask);

#define PINT_SERVREQ_GETATTR_FILL(__req,	\
				  __creds,	\
				  __fsid,	\
				  __handle,	\
				  __amask)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_GETATTR;		\
    (__req).credentials = (__creds);		\
    (__req).u.getattr.fs_id = (__fsid);		\
    (__req).u.getattr.handle = (__handle);	\
    (__req).u.getattr.attrmask = (__amask);	\
} while (0)

struct PVFS_servresp_getattr
{
    PVFS_object_attr attr;	    /* attributes */
};
endecode_fields_1_struct(PVFS_servresp_getattr, PVFS_object_attr, attr);
#define extra_size_PVFS_servresp_getattr extra_size_PVFS_object_attr

/* setattr ****************************************************/
/* - sets attributes specified by mask of PVFS_ATTR_XXX values */

struct PVFS_servreq_setattr
{
    PVFS_handle handle;		    /* handle of target object */
    PVFS_fs_id fs_id;		    /* file system */
    PVFS_object_attr attr;	    /* new attributes */
};
endecode_fields_3_struct(PVFS_servreq_setattr,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id,
  PVFS_object_attr, attr);
#define extra_size_PVFS_servreq_setattr extra_size_PVFS_object_attr

#define PINT_SERVREQ_SETATTR_FILL(__req,             \
                                  __creds,           \
                                  __fsid,            \
                                  __handle,          \
                                  __objtype,         \
                                  __attr,            \
                                  __amask)           \
do {                                                 \
    memset(&(__req), 0, sizeof(__req));              \
    (__req).op = PVFS_SERV_SETATTR;                  \
    (__req).credentials = (__creds);                 \
    (__req).u.setattr.fs_id = (__fsid);              \
    (__req).u.setattr.handle = (__handle);           \
    PINT_CONVERT_ATTR(&(__req).u.setattr.attr,       \
       &(__attr), PVFS_ATTR_COMMON_ALL);             \
    (__req).u.setattr.attr.objtype = (__objtype);    \
    (__req).u.setattr.attr.mask |= (__amask);        \
} while (0)

/* NOTE: no response structure; all necessary response info is 
 * returned in generic server response structure
 */

/* lookup path ************************************************/
/* - looks up as many elements of the specified path as possible */

struct PVFS_servreq_lookup_path
{
    char* path;			    /* path name */
    PVFS_fs_id fs_id;		    /* file system */
    PVFS_handle starting_handle;    /* handle of path parent */
    /* mask of attribs to return with lookup results */
    uint32_t attrmask;
};
endecode_fields_4_struct(PVFS_servreq_lookup_path,
  string, path,
  PVFS_fs_id, fs_id,
  PVFS_handle, starting_handle,
  uint32_t, attrmask);
#define extra_size_PVFS_servreq_lookup_path \
  roundup8(PVFS_REQ_LIMIT_PATH_NAME_BYTES + 1)

#define PINT_SERVREQ_LOOKUP_PATH_FILL(__req,		\
				      __creds,		\
				      __path,		\
				      __fsid,		\
				      __handle,		\
				      __amask)		\
do {							\
    memset(&(__req), 0, sizeof(__req));			\
    (__req).op = PVFS_SERV_LOOKUP_PATH;			\
    (__req).credentials = (__creds);			\
    (__req).u.lookup_path.path = (__path);		\
    (__req).u.lookup_path.fs_id = (__fsid);		\
    (__req).u.lookup_path.starting_handle = (__handle);	\
    (__req).u.lookup_path.attrmask = (__amask);		\
} while (0)

struct PVFS_servresp_lookup_path
{
    /* array of handles for each successfully resolved path segment */
    PVFS_handle *handle_array;	    
    /* array of attributes for each path segment (when available) */
    PVFS_object_attr *attr_array;
    uint32_t handle_count;	    /* # of handles returned */
    uint32_t attr_count;	    /* # of attributes returned */
};
endecode_fields_0aa_struct(PVFS_servresp_lookup_path,
  uint32_t, handle_count,
  PVFS_handle, handle_array,
  uint32_t, attr_count,
  PVFS_object_attr, attr_array);
/* this is a big thing that could be either a full path,
* or lots of handles, just use the max io req limit */
#define extra_size_PVFS_servresp_lookup_path (PVFS_REQ_LIMIT_IOREQ_BYTES)



/* mkdir *******************************************************/
/* - makes a new directory object */

struct PVFS_servreq_mkdir
{
    PVFS_fs_id fs_id;		    /* file system */
    PVFS_object_attr attr;	    /* initial attributes */

    /*
      an array of handle extents that we use to suggest to
      the server from which handle range to allocate for the
      newly created handle(s).  To request a single handle,
      a single extent with first = last should be used.
    */
    PVFS_handle_extent_array handle_extent_array;
};
endecode_fields_3_struct(PVFS_servreq_mkdir,
  PVFS_fs_id, fs_id,
  PVFS_object_attr, attr,
  PVFS_handle_extent_array, handle_extent_array);
#define extra_size_PVFS_servreq_mkdir \
    (PVFS_REQ_LIMIT_HANDLES_COUNT * sizeof(PVFS_handle_extent))

struct PVFS_servresp_mkdir
{
    PVFS_handle handle;		    /* handle of new directory */
};
endecode_fields_1_struct(PVFS_servresp_mkdir, PVFS_handle, handle);


/* create dirent ***********************************************/
/* - creates a new entry within an existing directory */

struct PVFS_servreq_crdirent
{
    char *name;			    /* name of new entry */
    PVFS_handle new_handle;	    /* handle of new entry */
    PVFS_handle parent_handle;	    /* handle of directory */
    PVFS_fs_id fs_id;		    /* file system */
};
endecode_fields_4_struct(PVFS_servreq_crdirent,
  string, name,
  PVFS_handle, new_handle,
  PVFS_handle, parent_handle,
  PVFS_fs_id, fs_id);
#define extra_size_PVFS_servreq_crdirent \
  roundup8(PVFS_REQ_LIMIT_SEGMENT_BYTES+1)

#define PINT_SERVREQ_CRDIRENT_FILL(__req,           \
                                   __creds,         \
                                   __name,          \
                                   __new_handle,    \
                                   __parent_handle, \
                                   __fs_id)         \
do {                                                \
    memset(&(__req), 0, sizeof(__req));             \
    (__req).op = PVFS_SERV_CRDIRENT;                \
    (__req).credentials = (__creds);                \
    (__req).u.crdirent.name = (__name);             \
    (__req).u.crdirent.new_handle = (__new_handle); \
    (__req).u.crdirent.parent_handle =              \
       (__parent_handle);                           \
    (__req).u.crdirent.fs_id = (__fs_id);           \
} while (0)

/* NOTE: no response structure; all necessary response info is 
 * returned in generic server response structure
 */

/* rmdirent ****************************************************/
/* - removes an existing directory entry */

struct PVFS_servreq_rmdirent
{
    char *entry;		    /* name of entry to remove */
    PVFS_handle parent_handle;	    /* handle of directory */
    PVFS_fs_id fs_id;		    /* file system */
};
endecode_fields_3_struct(PVFS_servreq_rmdirent,
  string, entry,
  PVFS_handle, parent_handle,
  PVFS_fs_id, fs_id);
#define extra_size_PVFS_servreq_rmdirent \
  roundup8(PVFS_REQ_LIMIT_SEGMENT_BYTES+1)

#define PINT_SERVREQ_RMDIRENT_FILL(__req,		\
				   __creds,		\
				   __fsid,		\
				   __handle,		\
				   __entry)		\
do {							\
    memset(&(__req), 0, sizeof(__req));			\
    (__req).op = PVFS_SERV_RMDIRENT;			\
    (__req).credentials = (__creds);			\
    (__req).u.rmdirent.fs_id = (__fsid);		\
    (__req).u.rmdirent.parent_handle = (__handle);	\
    (__req).u.rmdirent.entry = (__entry);	       	\
} while (0);


struct PVFS_servresp_rmdirent
{
    PVFS_handle entry_handle;	    /* handle of removed entry */
};
endecode_fields_1_struct(PVFS_servresp_rmdirent, PVFS_handle, entry_handle);

/* readdir *****************************************************/
/* - reads entries from a directory */

struct PVFS_servreq_readdir
{
    PVFS_handle handle;		    /* handle of dir object */
    PVFS_fs_id fs_id;		    /* file system */
    PVFS_ds_position token;	    /* dir offset */
    uint32_t dirent_count;	    /* desired # of entries */
};
endecode_fields_4_struct(PVFS_servreq_readdir,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id,
  PVFS_ds_position, token,
  uint32_t, dirent_count);

struct PVFS_servresp_readdir
{
    PVFS_ds_position token;	    /* new dir offset */
    /* array of directory entries */
    PVFS_dirent *dirent_array;
    uint32_t dirent_count;	    /* # of entries retrieved */
};
endecode_fields_1a_struct(PVFS_servresp_readdir,
  PVFS_ds_position, token,
  uint32_t, dirent_count,
  PVFS_dirent, dirent_array);
#define extra_size_PVFS_servresp_readdir \
  roundup8(PVFS_REQ_LIMIT_DIRENT_COUNT * (PVFS_NAME_MAX + 1 + 8))


/* getconfig ***************************************************/
/* - retrieves initial configuration information from server */

/* NOTE: no request structure; all necessary request info is
 * represented in generic server request structure
 */

struct PVFS_servresp_getconfig
{
    /* fs config data */
    char *fs_config_buf;
    /* size of fs config data */
    uint32_t fs_config_buf_size;
    /* server config data */
    char *server_config_buf;
    /* size of server specific config data */
    uint32_t server_config_buf_size;
};
endecode_fields_4_struct(PVFS_servresp_getconfig,
  string, fs_config_buf,
  uint32_t, fs_config_buf_size,
  string, server_config_buf,
  uint32_t, server_config_buf_size);
#define extra_size_PVFS_servresp_getconfig (2 * PVFS_REQ_LIMIT_CONFIG_FILE_BYTES)

/* truncate ****************************************************/
/* - resizes an existing datafile */

struct PVFS_servreq_truncate
{
    PVFS_handle handle;		    /* handle of obj to resize */
    PVFS_fs_id fs_id;		    /* file system */
    PVFS_size size;		    /* new size */
    int32_t flags;		    /* future use */

};
endecode_fields_4_struct(PVFS_servreq_truncate,
  PVFS_handle, handle,
  PVFS_fs_id, fs_id,
  PVFS_size, size,
  int32_t, flags);
#define PINT_SERVREQ_TRUNCATE_FILL(__req,		\
				__creds,	\
				__fsid,		\
				__size,         \
				__handle)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_TRUNCATE;		\
    (__req).credentials = (__creds);		\
    (__req).u.truncate.fs_id = (__fsid);	\
    (__req).u.truncate.size = (__size);         \
    (__req).u.truncate.handle = (__handle);	\
} while (0)

/* NOTE: no response structure; all necessary response info is 
 * returned in generic server response structure
 */

/* statfs ****************************************************/
/* - retrieves statistics for a particular file system */

struct PVFS_servreq_statfs
{
    PVFS_fs_id fs_id;		    /* file system */
};
endecode_fields_1_struct(PVFS_servreq_statfs, PVFS_fs_id, fs_id);

#define PINT_SERVREQ_STATFS_FILL(__req,		\
				__creds,	\
				__fsid)		\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_STATFS;		\
    (__req).credentials = (__creds);		\
    (__req).u.statfs.fs_id = (__fsid);		\
} while (0)

struct PVFS_servresp_statfs
{
    PVFS_statfs stat;
};
endecode_fields_1_struct(PVFS_servresp_statfs, PVFS_statfs, stat);

/* io **********************************************************/
/* - performs a read or write operation */

struct PVFS_servreq_io
{
    PVFS_handle handle;		    /* target datafile */
    PVFS_fs_id fs_id;		    /* file system */
    /* type of I/O operation to perform */
    enum PVFS_io_type io_type; /* enum defined in pvfs2-types.h */

    /* type of flow protocol to use for I/O transfer */
    enum PVFS_flowproto_type flow_type;

    /* relative number of this I/O server in distribution */
    uint32_t iod_num;
    /* total number of I/O servers involved in distribution */
    uint32_t iod_count;

    /* distribution */
    PVFS_Dist *io_dist;
    /* file datatype */
    PVFS_Request file_req;
    /* offset into file datatype */
    PVFS_offset file_req_offset;
    /* aggregate size of data to transfer */
    PVFS_size aggregate_size;
};
#ifdef __PINT_REQPROTO_ENCODE_FUNCS_C
#define encode_PVFS_servreq_io(pptr,x) do { \
    encode_PVFS_handle(pptr, &(x)->handle); \
    encode_PVFS_fs_id(pptr, &(x)->fs_id); \
    encode_enum(pptr, &(x)->io_type); \
    encode_enum(pptr, &(x)->flow_type); \
    encode_uint32_t(pptr, &(x)->iod_num); \
    encode_uint32_t(pptr, &(x)->iod_count); \
    encode_PVFS_Dist(pptr, &(x)->io_dist); \
    { \
    /* XXX: linearize into a fresh buffer, encode it, then throw it away;
     * later fix the structure so that it is easier to send. */ \
    struct PINT_Request *lin; \
    lin = decode_malloc(((x)->file_req->num_nested_req + 1) * sizeof(*lin)); \
    (void) PINT_Request_commit(lin, (x)->file_req); \
    encode_int32_t(pptr, &(x)->file_req->num_nested_req); \
    encode_PVFS_Request(pptr, lin); \
    decode_free(lin); \
    } \
    encode_PVFS_offset(pptr, &(x)->file_req_offset); \
    encode_PVFS_size(pptr, &(x)->aggregate_size); \
} while (0)
#define decode_PVFS_servreq_io(pptr,x) do { int numreq; \
    decode_PVFS_handle(pptr, &(x)->handle); \
    decode_PVFS_fs_id(pptr, &(x)->fs_id); \
    decode_enum(pptr, &(x)->io_type); \
    decode_enum(pptr, &(x)->flow_type); \
    decode_uint32_t(pptr, &(x)->iod_num); \
    decode_uint32_t(pptr, &(x)->iod_count); \
    decode_PVFS_Dist(pptr, &(x)->io_dist); \
    decode_int32_t(pptr, &numreq); \
    (x)->file_req = decode_malloc((numreq + 1) * sizeof(*(x)->file_req)); \
    (x)->file_req->num_nested_req = numreq; \
    decode_PVFS_Request(pptr, (x)->file_req); \
    PINT_Request_decode((x)->file_req); /* unpacks the pointers */ \
    decode_PVFS_offset(pptr, &(x)->file_req_offset); \
    decode_PVFS_size(pptr, &(x)->aggregate_size); \
} while (0)
/* could be huge, limit to max ioreq size beyond struct itself */
#define extra_size_PVFS_servreq_io roundup8(PVFS_REQ_LIMIT_PATH_NAME_BYTES)
#endif

#define PINT_SERVREQ_IO_FILL(__req,		\
			     __creds,		\
			     __fsid,		\
			     __handle,		\
			     __io_type,		\
			     __flow_type,	\
			     __datafile_nr,	\
			     __datafile_ct,	\
			     __io_dist,		\
			     __file_req,	\
			     __file_req_off,	\
			     __aggregate_size)  \
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op                 = PVFS_SERV_IO;	\
    (__req).credentials        = (__creds);	\
    (__req).u.io.fs_id         = (__fsid);	\
    (__req).u.io.handle        = (__handle);	\
    (__req).u.io.io_type       = (__io_type);	\
    (__req).u.io.flow_type     = (__flow_type);	\
    (__req).u.io.iod_num       = (__datafile_nr);	\
    (__req).u.io.iod_count     = (__datafile_ct);	\
    (__req).u.io.io_dist       = (__io_dist);	\
    (__req).u.io.file_req        = (__file_req);	\
    (__req).u.io.file_req_offset = (__file_req_off);	\
    (__req).u.io.aggregate_size  = (__aggregate_size);	\
} while (0)			     

struct PVFS_servresp_io
{
    PVFS_size bstream_size;	    /* size of datafile */
};
endecode_fields_1_struct(PVFS_servresp_io, PVFS_size, bstream_size);

/* write operations require a second response to announce completion */
struct PVFS_servresp_write_completion
{
    PVFS_size total_completed;	    /* amount of data transfered */
};
endecode_fields_1_struct(PVFS_servresp_write_completion,
  PVFS_size, total_completed);

/* mgmt_setparam ****************************************************/
/* - management operation for setting runtime parameters */

struct PVFS_servreq_mgmt_setparam
{
    PVFS_fs_id fs_id;		    /* file system */
    enum PVFS_server_param param;   /* paramter to set */
    int64_t value;		    /* parameter value */
};
endecode_fields_3_struct(PVFS_servreq_mgmt_setparam,
  PVFS_fs_id, fs_id,
  enum, param,
  int64_t, value);

#define PINT_SERVREQ_MGMT_SETPARAM_FILL(__req,	\
				  __creds,	\
				  __fsid,	\
				  __param,	\
				  __value)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_SETPARAM;	\
    (__req).credentials = (__creds);		\
    (__req).u.mgmt_setparam.fs_id = (__fsid);	\
    (__req).u.mgmt_setparam.param = (__param);	\
    (__req).u.mgmt_setparam.value = (__value);	\
} while (0)

struct PVFS_servresp_mgmt_setparam
{
    int64_t old_value;
};
endecode_fields_1_struct(PVFS_servresp_mgmt_setparam, int64_t, old_value);

/* mgmt_noop ********************************************************/
/* - does nothing except contact a server to see if it is responding
 * to requests
 */

/* NOTE: not request or response structure; all necessary information
 * is contained in generic server request and response structures
 */

#define PINT_SERVREQ_MGMT_NOOP_FILL(__req,	\
				  __creds)	\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_NOOP;		\
    (__req).credentials = (__creds);		\
} while (0)


/* mgmt_perf_mon ****************************************************/
/* retrieves performance statistics from server */

struct PVFS_servreq_mgmt_perf_mon
{
    uint32_t next_id;  /* next time stamp id we want to retrieve */
    uint32_t count;    /* how many measurements we want */
};
endecode_fields_2_struct(PVFS_servreq_mgmt_perf_mon,
  uint32_t, next_id,
  uint32_t, count);

#define PINT_SERVREQ_MGMT_PERF_MON_FILL(__req,	\
					__creds,\
					__next_id,\
					__count)\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_PERF_MON;	\
    (__req).credentials = (__creds);		\
    (__req).u.mgmt_perf_mon.next_id = (__next_id); \
    (__req).u.mgmt_perf_mon.count = (__count);	\
} while (0)

struct PVFS_servresp_mgmt_perf_mon
{
    struct PVFS_mgmt_perf_stat* perf_array;	/* array of statistics */
    uint32_t perf_array_count;		/* size of above array */
    uint32_t suggested_next_id;		/* next id to pick up from this point */
    uint64_t end_time_ms;		/* end time for final array entry */
    uint64_t cur_time_ms;		/* current time according to svr */
};
endecode_fields_3a_struct(PVFS_servresp_mgmt_perf_mon,
  uint32_t, suggested_next_id,
  uint64_t, end_time_ms,
  uint64_t, cur_time_ms,
  uint32_t, perf_array_count,
  PVFS_mgmt_perf_stat, perf_array);
#define extra_size_PVFS_servresp_mgmt_perf_mon (PVFS_REQ_LIMIT_IOREQ_BYTES)

/* mgmt_iterate_handles ***************************************/
/* iterates through handles stored on server */

struct PVFS_servreq_mgmt_iterate_handles
{
    PVFS_fs_id fs_id;
    int32_t handle_count;
    PVFS_ds_position position;
};
endecode_fields_3_struct(PVFS_servreq_mgmt_iterate_handles,
  PVFS_fs_id, fs_id,
  int32_t, handle_count,
  PVFS_ds_position, position);

#define PINT_SERVREQ_MGMT_ITERATE_HANDLES_FILL(__req,	\
					__creds,\
					__fs_id,\
					__handle_count,\
					__position)\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_ITERATE_HANDLES;\
    (__req).credentials = (__creds);		\
    (__req).u.mgmt_iterate_handles.fs_id = (__fs_id); \
    (__req).u.mgmt_iterate_handles.handle_count = (__handle_count); \
    (__req).u.mgmt_iterate_handles.position = (__position); \
} while (0)


struct PVFS_servresp_mgmt_iterate_handles
{
    PVFS_ds_position position;
    PVFS_handle* handle_array;
    int handle_count;
};
endecode_fields_1a_struct(PVFS_servresp_mgmt_iterate_handles,
  PVFS_ds_position, position,
  int32_t, handle_count,
  PVFS_handle, handle_array);
#define extra_size_PVFS_servresp_mgmt_iterate_handles \
  (PVFS_REQ_LIMIT_HANDLES_COUNT * sizeof(PVFS_handle))

/* mgmt_dspace_info_list **************************************/
/* - returns low level dspace information for a list of handles */

struct PVFS_servreq_mgmt_dspace_info_list
{
    PVFS_fs_id fs_id;
    PVFS_handle* handle_array;
    int32_t handle_count;
};
endecode_fields_1a_struct(PVFS_servreq_mgmt_dspace_info_list,
  PVFS_fs_id, fs_id,
  int32_t, handle_count,
  PVFS_handle, handle_array);
#define extra_size_PVFS_servreq_mgmt_dspace_info_list \
  (PVFS_REQ_LIMIT_HANDLES_COUNT * sizeof(PVFS_handle))

#define PINT_SERVREQ_MGMT_DSPACE_INFO_LIST(__req,	\
					__creds,\
					__fs_id,\
					__handle_array,\
					__handle_count)\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_DSPACE_INFO_LIST;\
    (__req).credentials = (__creds);		\
    (__req).u.mgmt_dspace_info_list.fs_id = (__fs_id); \
    (__req).u.mgmt_dspace_info_list.handle_array = (__handle_array); \
    (__req).u.mgmt_dspace_info_list.handle_count = (__handle_count); \
} while (0)

struct PVFS_servresp_mgmt_dspace_info_list
{
    struct PVFS_mgmt_dspace_info* dspace_info_array;
    int32_t dspace_info_count;
};
endecode_fields_0a_struct(PVFS_servresp_mgmt_dspace_info_list,
  int32_t, dspace_info_count,
  PVFS_mgmt_dspace_info, dspace_info_array);
#define extra_size_PVFS_servresp_mgmt_dspace_info_list \
  (PVFS_REQ_LIMIT_MGMT_DSPACE_INFO_LIST_COUNT * sizeof(struct PVFS_mgmt_dspace_info))

/* mgmt_event_mon **************************************/
/* - returns event logging data */

struct PVFS_servreq_mgmt_event_mon
{
    uint32_t event_count;
};
endecode_fields_1_struct(PVFS_servreq_mgmt_event_mon,
    uint32_t, event_count);

#define PINT_SERVREQ_MGMT_EVENT_MON_FILL(__req,	\
					__creds,\
					__event_count)\
do {						\
    memset(&(__req), 0, sizeof(__req));		\
    (__req).op = PVFS_SERV_MGMT_EVENT_MON;\
    (__req).credentials = (__creds);		\
    (__req).u.mgmt_event_mon.event_count = (__event_count); \
} while (0)

struct PVFS_servresp_mgmt_event_mon
{
    struct PVFS_mgmt_event* event_array;
    uint32_t event_count;
};
endecode_fields_0a_struct(PVFS_servresp_mgmt_event_mon,
    uint32_t, event_count,
    PVFS_mgmt_event, event_array);

/* server request *********************************************/
/* - generic request with union of all op specific structs */

enum PVFS_server_req_flags
{
    PVFS_SERVER_REQ_ADMIN_MODE = 1
};

struct PVFS_server_req
{
    enum PVFS_server_op op;
    PVFS_credentials credentials;
    int32_t flags;
    union
    {
	struct PVFS_servreq_create create;
	struct PVFS_servreq_remove remove;
	struct PVFS_servreq_io io;
	struct PVFS_servreq_getattr getattr;
	struct PVFS_servreq_setattr setattr;
	struct PVFS_servreq_mkdir mkdir;
	struct PVFS_servreq_readdir readdir;
	struct PVFS_servreq_lookup_path lookup_path;
	struct PVFS_servreq_crdirent crdirent;
	struct PVFS_servreq_rmdirent rmdirent;
	struct PVFS_servreq_truncate truncate;
	struct PVFS_servreq_flush flush;
	struct PVFS_servreq_mgmt_setparam mgmt_setparam;
	struct PVFS_servreq_statfs statfs;
	struct PVFS_servreq_mgmt_perf_mon mgmt_perf_mon;
	struct PVFS_servreq_mgmt_iterate_handles mgmt_iterate_handles;
	struct PVFS_servreq_mgmt_dspace_info_list mgmt_dspace_info_list;
	struct PVFS_servreq_mgmt_event_mon mgmt_event_mon;
    }
    u;
};
endecode_fields_2_struct(PVFS_server_req,
  enum, op,
  PVFS_credentials, credentials);

/* server response *********************************************/
/* - generic response with union of all op specific structs */
struct PVFS_server_resp
{
    enum PVFS_server_op op;
    PVFS_error status;
    union
    {
	struct PVFS_servresp_create create;
	struct PVFS_servresp_getattr getattr;
	struct PVFS_servresp_mkdir mkdir;
	struct PVFS_servresp_readdir readdir;
	struct PVFS_servresp_lookup_path lookup_path;
	struct PVFS_servresp_rmdirent rmdirent;
	struct PVFS_servresp_getconfig getconfig;
	struct PVFS_servresp_io io;
	struct PVFS_servresp_write_completion write_completion;
	struct PVFS_servresp_statfs statfs;
	struct PVFS_servresp_mgmt_setparam mgmt_setparam;
	struct PVFS_servresp_mgmt_perf_mon mgmt_perf_mon;
	struct PVFS_servresp_mgmt_iterate_handles mgmt_iterate_handles;
	struct PVFS_servresp_mgmt_dspace_info_list mgmt_dspace_info_list;
	struct PVFS_servresp_mgmt_event_mon mgmt_event_mon;
    }
    u;
};
endecode_fields_2_struct(PVFS_server_resp,
  enum, op,
  PVFS_error, status);

#endif /* __PVFS2_REQ_PROTO_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
