/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/* This file defines the API for encoding and decoding the request
 * protocol that is used between clients and servers in pvfs2
 */

#ifndef __PINT_REQUEST_ENCODE_H
#define __PINT_REQUEST_ENCODE_H

#include "bmi.h"

/* structure to describe messages that have been encoded */
struct PINT_encoded_msg
{
    bmi_addr_t dest;			    /* host this is going to */
    enum bmi_buffer_type buffer_type;	    /* buffer flag for BMI's use */
    void** buffer_list;			    /* list of buffers */
    PVFS_size* size_list;		    /* size of buffers */
    int list_count;			    /* number of buffers */
    PVFS_size total_size;		    /* aggregate size of encoding */
    int type;
};

/* structure to describe messages that have been decoded */
struct PINT_decoded_msg
{
    void* buffer;
};

/* types of messages we will encode or decode */
enum PINT_encode_msg_type
{
    PINT_ENCODE_REQ = 7,
    PINT_DECODE_REQ = 7,
    PINT_ENCODE_RESP = 13,
    PINT_DECODE_RESP = 13
};

/*******************************************************
 * public function prototypes
 */

/* PINT_encode()
 * 
 * encodes a buffer (containing a PVFS2 request or response) to be
 * sent over the network
 * 
 * returns 0 on success, -ERRNO on failure
 */
int PINT_encode(
		void* input_buffer,
		enum PINT_encode_msg_type input_type,
		struct PINT_encoded_msg* target_msg,
		bmi_addr_t target_addr,
		int type
		);

/* PINT_decode()
 *
 * decodes a buffer (containing a PVFS2 request or response) that
 * has been received from the network
 *
 * returns 0 on success, -ERRNO on failure
 */
int PINT_decode(
		void* input_buffer,
		enum PINT_encode_msg_type input_type,
		struct PINT_decoded_msg* target_msg,
		bmi_addr_t target_addr,
		PVFS_size size,
		int *type
		);
	
/* PINT_encode_release()
 *
 * frees all resources associated with a message that has been
 * encoded 
 *
 * no return value
 */
void PINT_encode_release(
			 struct PINT_encoded_msg* msg,
			 enum PINT_encode_msg_type input_type,
			 int type
			 );

/* PINT_decode_release()
 *
 * frees all resources associated with a message that has been
 * decoded
 *
 * no return value
 */
void PINT_decode_release(
			 struct PINT_decoded_msg* msg,
			 enum PINT_encode_msg_type input_type,
			 int type
			 );

/* PINT_get_encoded_generic_ack_sz(int type, int op)
 *
 * frees all resources associated with a message that has been
 * decoded
 *
 * returns size of encoded generic ack.
 */
int PINT_get_encoded_generic_ack_sz(
			 int type,
			 int op
			 );


#endif /* __PINT_REQUEST_ENCODE_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
