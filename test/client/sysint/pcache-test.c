#include "pcache.h"
#include "pcache-test.h"

void print_pinode(pinode *toprint);

int main(int argc,char* argv[])
{
	pcache test_pcache;
	pinode *pinode1, *pinode2, *pinode3, *test_pinode = NULL;
	int ret;

	ret = pcache_initialize( &test_pcache );
	if (ret < 0)
	{
		printf("pcache init failed with errcode %d\n", ret);
		return(-1);
	}

	pinode1 = (pinode *)malloc(sizeof(pinode));
	if (pinode1 == NULL)
	{
		printf("malloc pinode 1 failed\n");
		return(-1);
	}

	pinode2 = (pinode *)malloc(sizeof(pinode));
	if (pinode2 == NULL)
	{
		printf("malloc pinode 2 failed\n");
		return(-1);
	}

	pinode3 = (pinode *)malloc(sizeof(pinode));
	if (pinode3 == NULL)
	{
		printf("malloc pinode 3 failed\n");
		return(-1);
	}

	/* insert some elements */

	pinode1->pinode_ref.handle = 1;
	pinode1->pinode_ref.fs_id = 1;
	pinode1->attr.owner = 1;
	pinode1->attr.group = 1;
	pinode1->attr.perms = 1;
	pinode1->attr.ctime = 1;
	pinode1->attr.mtime = 1;
	pinode1->attr.atime = 1;
	pinode1->attr.objtype = ATTR_META;
	//pinode1->attr.u.meta = ;
	//pinode1->attr.u.data = ;
	pinode1->mask = ATTR_META;
	pinode1->size = 1;
	pinode1->tstamp_handle.tv_sec = 111;
	pinode1->tstamp_handle.tv_usec = 111;
	pinode1->tstamp_attr.tv_sec = 111111;
	pinode1->tstamp_attr.tv_usec = 111111;
	pinode1->tstamp_size.tv_sec = 111111111;
	pinode1->tstamp_size.tv_usec = 111111111;

	pinode2->pinode_ref.handle = 2;
	pinode2->pinode_ref.fs_id = 2;
	pinode2->attr.owner = 2;
	pinode2->attr.group = 2;
	pinode2->attr.perms = 2;
	pinode2->attr.ctime = 2;
	pinode2->attr.mtime = 2;
	pinode2->attr.atime = 2;
	pinode2->attr.objtype = ATTR_META;
	//pinode2->attr.u.meta = ;
	//pinode2->attr.u.data = ;
	pinode2->mask = ATTR_META;
	pinode2->size = 2;
	pinode2->tstamp_handle.tv_sec = 222;
	pinode2->tstamp_handle.tv_usec = 222;
	pinode2->tstamp_attr.tv_usec = 222222;
	pinode2->tstamp_attr.tv_usec = 222222;
	pinode2->tstamp_size.tv_sec = 222222222;
	pinode2->tstamp_size.tv_usec = 222222222;

	pinode3->pinode_ref.handle = 3;
	pinode3->pinode_ref.fs_id = 3;
	pinode3->attr.owner = 3;
	pinode3->attr.group = 3;
	pinode3->attr.perms = 3;
	pinode3->attr.ctime = 3;
	pinode3->attr.mtime = 3;
	pinode3->attr.atime = 3;
	pinode3->attr.objtype = ATTR_META;
	//pinode3->attr.u.meta = ;
	//pinode3->attr.u.data = ;
	pinode3->mask = ATTR_META;
	pinode3->size = 3;
	pinode3->tstamp_handle.tv_sec = 333;
	pinode3->tstamp_handle.tv_usec = 333;
	pinode3->tstamp_attr.tv_sec = 333333;
	pinode3->tstamp_attr.tv_usec = 333333;
	pinode3->tstamp_size.tv_sec = 333333333;
	pinode3->tstamp_size.tv_usec = 333333333;

	ret = pcache_insert(&test_pcache, pinode1);
	if (ret < 0)
	{
		printf("pcache insert failed (#1) with errcode %d\n", ret);
		return(-1);
	}

	ret = pcache_insert(&test_pcache, pinode2);
	if (ret < 0)
	{
		printf("pcache insert failed (#2) with errcode %d\n", ret);
		return(-1);
	}

	ret = pcache_insert(&test_pcache, pinode3);
	if (ret < 0)
	{
		printf("pcache insert failed (#3) with errcode %d\n", ret);
		return(-1);
	}

	/* lookup element that was inserted */

	ret = pcache_lookup(&test_pcache, pinode2->pinode_ref, test_pinode);
	if (ret < 0)
	{
		printf("pcache lookup failed (#2) with errcode %d\n", ret);
		return(-1);
	}

	print_pinode( test_pinode );

	/* remove an element */
	/* lookup element that was removed */

	ret = pcache_finalize( test_pcache );
	if (ret < 0)
	{
		printf("pcache finalize failed with errcode %d\n", ret);
		return(-1);
	}

	return(0);
}

void print_pinode(pinode *toprint)
{
	printf("============printing=pnode=========\n");
	printf("pinode.pinode_ref.handle = %d\n", (int)toprint->pinode_ref.handle);
	printf("pinode.pinode_ref.fs_id = %d\n", (int)toprint->pinode_ref.fs_id);
	printf("pinode.attr.owner = %d\n", (int)toprint->attr.owner);
	printf("pinode.attr.group = %d\n", (int)toprint->attr.group);
	printf("pinode.attr.perms = %d\n", (int)toprint->attr.perms);
	printf("pinode.attr.ctime = %d\n", (int)toprint->attr.ctime);
	printf("pinode.attr.mtime = %d\n", (int)toprint->attr.mtime);
	printf("pinode.attr.atime = %d\n", (int)toprint->attr.atime);
	switch(toprint->attr.objtype)
	{
		case ATTR_META:
			printf("pinode.attr.objtype = ATTR_META\n");
		default:
			printf("pinode.attr.objtype = dunno\n");
			break;
	}
	printf("pinode.mask = %d\n", (int)toprint->mask);
	printf("pinode.size = %d\n", (int)toprint->size);
	printf("pinode.tstamp_handle.tv_sec = %d\n", (int)toprint->tstamp_handle.tv_sec);
	printf("pinode.tstamp_handle.tv_usec = %d\n", (int)toprint->tstamp_handle.tv_usec);
	printf("pinode.tstamp_handle.tv_sec = %d\n", (int)toprint->tstamp_handle.tv_sec);
	printf("pinode.tstamp_handle.tv_usec = %d\n", (int)toprint->tstamp_handle.tv_usec);
	printf("pinode.tstamp_attr.tv_sec = %d\n", (int)toprint->tstamp_attr.tv_sec);
	printf("pinode.tstamp_attr.tv_usec = %d\n", (int)toprint->tstamp_attr.tv_usec);
	printf("pinode.tstamp_size.tv_secc = %d\n", (int)toprint->tstamp_size.tv_sec);
	printf("pinode.tstamp_size.tv_usec = %d\n", (int)toprint->tstamp_size.tv_usec);
	printf("===================================\n");
}

