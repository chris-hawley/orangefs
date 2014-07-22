/*
 * Copyright � 2014 Omnibond Systems LLC
 *
 * See COPYING in top-level directory.
 *
 * Client-side capability cache implementation
 *
 */
  
#include <string.h>

#include "pvfs2-types.h"
#include "security-util.h"
#include "pint-util.h"
#include "pvfs2-debug.h"
#include "gossip.h"
#include "pvfs2-internal.h"

#include "client-capcache.h"
  
/* compile time defaults */
/* the default is high because it isn't used--the cap timeout
   is used */
#define CLIENT_CAPCACHE_DEFAULT_TIMEOUT_MSECS 1000000000LL
#define CLIENT_CAPCACHE_DEFAULT_SOFT_LIMIT 5120
#define CLIENT_CAPCACHE_DEFAULT_HARD_LIMIT 10240
#define CLIENT_CAPCACHE_DEFAULT_RECLAIM_PERCENTAGE 25
#define CLIENT_CAPCACHE_DEFAULT_REPLACE_ALGORITHM LEAST_RECENTLY_USED

struct PINT_perf_key client_capcache_keys[] = 
{
   {"CLIENT_CAPCACHE_NUM_ENTRIES", PERF_CLIENT_CAPCACHE_NUM_ENTRIES, 
       PINT_PERF_PRESERVE},
   {"CLIENT_CAPCACHE_SOFT_LIMIT", PERF_CLIENT_CAPCACHE_SOFT_LIMIT, 
       PINT_PERF_PRESERVE},
   {"CLIENT_CAPCACHE_HARD_LIMIT", PERF_CLIENT_CAPCACHE_HARD_LIMIT, 
       PINT_PERF_PRESERVE},
   {"CLIENT_CAPCACHE_HITS", PERF_CLIENT_CAPCACHE_HITS, 0},
   {"CLIENT_CAPCACHE_MISSES", PERF_CLIENT_CAPCACHE_MISSES, 0},
   {"CLIENT_CAPCACHE_UPDATES", PERF_CLIENT_CAPCACHE_UPDATES, 0},
   {"CLIENT_CAPCACHE_PURGES", PERF_CLIENT_CAPCACHE_PURGES, 0},
   {"CLIENT_CAPCACHE_REPLACEMENTS", PERF_CLIENT_CAPCACHE_REPLACEMENTS, 0},
   {"CLIENT_CAPCACHE_DELETIONS", PERF_CLIENT_CAPCACHE_DELETIONS, 0},
   {"CLIENT_CAPCACHE_ENABLED", PERF_CLIENT_CAPCACHE_ENABLED, 
       PINT_PERF_PRESERVE},
   {NULL, 0, 0},
};

/* data to be stored in a cached entry */
struct client_capcache_payload
{
    PVFS_object_ref refn;    /* PVFS2 object reference */
    PVFS_uid uid;            /* user ID */
    PVFS_capability cap;     /* cached capability */
};

/* lookup key - indexed by object reference (handle / fs_id) and
   user ID */
struct client_capcache_key
{
    PVFS_object_ref refn;
    PVFS_uid uid;
};

static struct PINT_tcache *client_capcache = NULL;
static gen_mutex_t client_capcache_mutex = GEN_MUTEX_INITIALIZER;
  
static int client_capcache_compare_key_entry(const void *key, struct qhash_head *link);
static int client_capcache_free_payload(void *payload);

static int client_capcache_hash_key(const void *key, int table_size);
static struct PINT_perf_counter *client_capcache_pc = NULL;

static int set_client_capcache_defaults(struct PINT_tcache *instance);

/**
 * Enables perf counter instrumentation of the client_capcache
 */
void PINT_client_capcache_enable_perf_counter(
    struct PINT_perf_counter *pc_in) /**< counter for cache fields */
{
    if (pc_in == NULL)
        return;

    gen_mutex_lock(&client_capcache_mutex);

    client_capcache_pc = pc_in;

    /* set initial values */
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_SOFT_LIMIT,
        client_capcache->soft_limit, PINT_PERF_SET);
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_HARD_LIMIT,
        client_capcache->hard_limit, PINT_PERF_SET);
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_ENABLED,
        client_capcache->enable, PINT_PERF_SET);

    gen_mutex_unlock(&client_capcache_mutex);
    return;
}

/**
 * Initializes the client_capcache 
 * \return pointer to tcache on success, NULL on failure
 */
int PINT_client_capcache_initialize(void)
{
    int ret = -1;
  
    gen_mutex_lock(&client_capcache_mutex);
  
    /* create tcache instances */
    client_capcache = PINT_tcache_initialize(client_capcache_compare_key_entry,
                                             client_capcache_hash_key,
                                             client_capcache_free_payload,
                                             -1 /* default tcache size */);
    if (client_capcache == NULL)
    {
        gen_mutex_unlock(&client_capcache_mutex);
        return -PVFS_ENOMEM;
    }

    /* fill in defaults that are common to both */
    ret = set_client_capcache_defaults(client_capcache);
    if (ret < 0)
    {
        PINT_tcache_finalize(client_capcache);
        /* PINT_tcache_finalize(static_client_capcache); */
        gen_mutex_unlock(&client_capcache_mutex);
        return ret;
    }
 
    gen_mutex_unlock(&client_capcache_mutex);
    return 0;
}
  
/** Finalizes and destroys the client_capcache, frees all cached entries */
void PINT_client_capcache_finalize(void)
{
    gen_mutex_lock(&client_capcache_mutex);

    PINT_tcache_finalize(client_capcache);
    client_capcache = NULL;

    gen_mutex_unlock(&client_capcache_mutex);
    return;
}
  
/**
 * Retrieves parameters from the client_capcache 
 * @see PINT_tcache_options
 * \return 0 on success, -PVFS_error on failure
 */
int PINT_client_capcache_get_info(
    enum PINT_client_capcache_options option, /**< option to read */
    unsigned int *arg)                   /**< output value */
{
    int ret = -1;
    
    gen_mutex_lock(&client_capcache_mutex);

    ret = PINT_tcache_get_info(client_capcache, option, arg);
  
    gen_mutex_unlock(&client_capcache_mutex);
  
    return ret;
}

/**
 * Sets optional parameters in the client_capcache
 * @see PINT_tcache_options
 * @return 0 on success, -PVFS_error on failure
 */
int PINT_client_capcache_set_info(
    enum PINT_client_capcache_options option, /**< option to modify */
    unsigned int arg)             /**< input value */
{
    int ret = -1;
  
    gen_mutex_lock(&client_capcache_mutex);

    ret = PINT_tcache_set_info(client_capcache, option, arg);

    /* record any parameter changes that may have resulted*/
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_SOFT_LIMIT,
        client_capcache->soft_limit, PINT_PERF_SET);
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_HARD_LIMIT,
        client_capcache->hard_limit, PINT_PERF_SET);
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_ENABLED,
        client_capcache->enable, PINT_PERF_SET);
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_NUM_ENTRIES,
        client_capcache->num_entries, PINT_PERF_SET);
    
    gen_mutex_unlock(&client_capcache_mutex);

    return(ret);
}

/**
 * Retrieves cached capabilty; caller must free when no longer needed
 *
 * return 0 on success, -PVFS_error on failure
 */
int PINT_client_capcache_get_cached_entry(
    PVFS_object_ref refn,
    PVFS_uid uid,
    PVFS_capability *cap)
{
    int ret = -1;
    struct client_capcache_key key;
    struct PINT_tcache_entry *tmp_entry = NULL;
    struct client_capcache_payload *tmp_payload = NULL;
    int status;
    struct timeval current_time = { 0, 0 };
  
    gossip_debug(GOSSIP_SECURITY_DEBUG, "%s: H=%llu uid=%d\n",
                 __func__, llu(refn.handle), uid);
    
    gen_mutex_lock(&client_capcache_mutex);
  
    /* lookup */
    key.refn = refn;
    key.uid = uid;
    ret = PINT_tcache_lookup(client_capcache, &key, &tmp_entry, &status);
    if (ret < 0 || status != 0)
    {
        PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_MISSES, 1, PINT_PERF_ADD);
        gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache: miss: H=%llu uid=%d\n",
                     llu(refn.handle), uid);
    }
    else
    {
        PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_HITS, 1, PINT_PERF_ADD);
        tmp_payload = tmp_entry->payload;
    }

    if (tmp_payload == NULL)
    {
        /* missed everything */
        gen_mutex_unlock(&client_capcache_mutex);
        return ret;
    }

    /* Get the time of day */
    gettimeofday(&current_time, NULL);

    /* Check to see if cap has expired 
       TODO: buffer time? */
    if (current_time.tv_sec <= (time_t) tmp_payload->cap.timeout)
    {
        gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache: hit: H=%llu uid=%d\n",
                     llu(refn.handle), uid);
        /* copy capability */
        ret = PINT_copy_capability((const PVFS_capability *) &tmp_payload->cap, 
                                   cap);
    }
    else
    {
        ret = -PVFS_ETIME;
        gossip_debug(GOSSIP_CLIENT_DEBUG, "client_capcache: hit: H=%llu uid=%d "
                     "EXPIRED\n", llu(refn.handle), uid);
        /* TODO: remove from cache here? */

    }
    
    gen_mutex_unlock(&client_capcache_mutex);
  
    return ret;
}

/**
 * Invalidates a cache entry (if present)
 */
void PINT_client_capcache_invalidate(
    PVFS_object_ref refn,
    PVFS_uid uid)
{
    int ret = -1;
    struct client_capcache_key key;
    struct PINT_tcache_entry *tmp_entry;
    int tmp_status;
  
    gossip_debug(GOSSIP_SECURITY_DEBUG, "%s: H=%llu uid=%d\n",
                 __func__, llu(refn.handle), uid);
  
    gen_mutex_lock(&client_capcache_mutex);
  
    /* find out if we have non-static items cached */
    key.refn = refn;
    key.uid = uid;
    ret = PINT_tcache_lookup(client_capcache, 
                             &key,
                             &tmp_entry,
                             &tmp_status);
    if (ret == 0)
    {
        PINT_tcache_delete(client_capcache, tmp_entry);
        PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_DELETIONS, 1,
                        PINT_PERF_ADD);
    }
  
    /* set the new current number of entries */
    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_NUM_ENTRIES,
                    client_capcache->num_entries, PINT_PERF_SET);

    gen_mutex_unlock(&client_capcache_mutex);

    return;
}
  
/**
 * Add capability to cache
 *
 * return 0 on success, -PVFS_error on failure
 */
int PINT_client_capcache_update(
    PVFS_object_ref refn, 
    PVFS_uid uid,
    const PVFS_capability *cap)
{
    int ret = -1, ret2, status, purged;
    struct client_capcache_key key;
    struct client_capcache_payload *tmp_payload = NULL;
    struct PINT_tcache_entry *tmp_entry;
    struct timeval timev = { 0, 0 }, now = { 0, 0 };

    if (cap == NULL)
    {
        return -PVFS_EINVAL;
    }

    gossip_debug(GOSSIP_SECURITY_DEBUG, "%s: H=%llu uid=%d\n",
                 __func__, llu(refn.handle), uid);

    /* don't cache expired cap TODO: buffer? */
    gettimeofday(&timev, NULL);
    if (timev.tv_sec > cap->timeout)
    {
        gen_mutex_unlock(&client_capcache_mutex);
        gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache: not caching "
                     "expired cap (%llu > %llu)\n", llu(timev.tv_sec), 
                     llu(cap->timeout));
        return -PVFS_ETIME;
    }

    gen_mutex_lock(&client_capcache_mutex);

    /* find out if the entry is already in the cache */
    key.refn = refn;
    key.uid = uid;
    ret = PINT_tcache_lookup(client_capcache, 
                             &key,
                             &tmp_entry,
                             &status);

    if (ret == 0)
    {
        PVFS_time tmp_timeout;

        gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: entry "
                     "found H=%llu uid=%d\n", llu(refn.handle), uid);
        
        /* only update the entry if the timeout is newer TODO: buffer? */
        ret = -1;
        tmp_timeout = ((struct client_capcache_payload *) 
                       tmp_entry->payload)->cap.timeout;
        if (cap->timeout > tmp_timeout)
        {
            ret = PINT_tcache_delete(client_capcache, tmp_entry);
        }

        if (ret == 0)
        {
            gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: "
                         "updating entry H=%llu uid=%d\n", llu(refn.handle), uid);
            /* build new payload */
            tmp_payload = (struct client_capcache_payload *) 
                              calloc(1, sizeof(*tmp_payload));
            if (tmp_payload == NULL)
            {
                /* TODO: gossip */
                gen_mutex_unlock(&client_capcache_mutex);
                return -PVFS_ENOMEM;
            }
            tmp_payload->refn = refn;
            tmp_payload->uid = uid;
            if ((ret2 = PINT_copy_capability(cap, &tmp_payload->cap)) != 0)
            {
                gossip_err("client_capcache update: could not copy capability\n");
                gen_mutex_unlock(&client_capcache_mutex);
                return ret2;
            }

            timev.tv_sec = cap->timeout;
            timev.tv_usec = 0;
            ret = PINT_tcache_insert_entry_ex(client_capcache, &key, tmp_payload,
                                          &timev, &purged);

            /* this counts as an update of an existing entry */
            PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_UPDATES,
                            1, PINT_PERF_ADD);
        }
        else if (ret == -1)
        {
            gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: "
                         "ignoring update as timeout is not later (%llu <= %llu)"
                         " H=%llu uid=%d\n", llu(cap->timeout), llu(tmp_timeout),
                         llu(refn.handle), uid);
            ret = 0;
        }
        else
        {
            gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: could "
                         "not remove entry (%d) H=%llu uid=%d\n", ret,
                         llu(refn.handle), uid);
        }
    }
    else
    {
        gettimeofday(&now, NULL);
        gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: inserting "
                     "new entry H=%llu uid=%d now=%d timeout=%d\n", llu(refn.handle),
                     uid, now.tv_sec, cap->timeout);
        /* not found in cache; insert new payload*/
        timev.tv_sec = cap->timeout;
        timev.tv_usec = 0;
        /* build new payload */
        tmp_payload = (struct client_capcache_payload *) 
                          calloc(1, sizeof(*tmp_payload));
        if (tmp_payload == NULL)
        {
            /* TODO: gossip */
            gen_mutex_unlock(&client_capcache_mutex);
            return -PVFS_ENOMEM;
        }
        tmp_payload->refn = refn;
        tmp_payload->uid = uid;
        PINT_copy_capability(cap, &tmp_payload->cap);
        /* TODO: error-checking */
        ret = PINT_tcache_insert_entry_ex(client_capcache, &key, tmp_payload,
                                          &timev, &purged);
        /* the purged variable indicates how many entries had to be purged
         * from the tcache to make room for this new one
         */
        if (purged == 1)
        {
            /* since only one item was purged, we count this as one item being
             * replaced rather than as a purge and an insert 
             */
            PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_REPLACEMENTS,
                            purged, PINT_PERF_ADD);
        }
        else
        {
            /* otherwise we just purged as part of reclaimation */
            /* if we didn't purge anything, then the "purged" variable will
             * be zero and this counter call won't do anything.
             */
            PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_PURGES, 
                            purged, PINT_PERF_ADD);
        }
    }

    PINT_perf_count(client_capcache_pc, PERF_CLIENT_CAPCACHE_NUM_ENTRIES,
                    client_capcache->num_entries, PINT_PERF_SET);

    gen_mutex_unlock(&client_capcache_mutex);

    gossip_debug(GOSSIP_SECURITY_DEBUG, "client_capcache update: returning %d\n",
                 ret);
  
    return ret;
}

/* client_capcache_compare_key_entry()
 *
 * compares an opaque key (object ref in this case) against a payload to see
 * if there is a match
 *
 * returns 1 on match, 0 otherwise
 */
static int client_capcache_compare_key_entry(const void *key, struct qhash_head *link)
{
    struct client_capcache_key *real_key = (struct client_capcache_key *) key;
    struct client_capcache_payload *tmp_payload = NULL;
    struct PINT_tcache_entry *tmp_entry = NULL;
  
    tmp_entry = qhash_entry(link, struct PINT_tcache_entry, hash_link);
    if (tmp_entry == NULL)
    {
        return 0;
    }
  
    tmp_payload = (struct client_capcache_payload *) tmp_entry->payload;
    /* TODO: temp */
    gossip_debug(GOSSIP_SECURITY_DEBUG, "%s: checking payload: H=%llu F=%llu uid=%d\n",
                 __func__, llu(tmp_payload->refn.handle), llu(tmp_payload->refn.fs_id), tmp_payload->uid);
    if (real_key->uid == tmp_payload->uid &&
        real_key->refn.handle == tmp_payload->refn.handle &&
        real_key->refn.fs_id == tmp_payload->refn.fs_id)
    {
        return 1;
    }
  
    return 0;
}

/* client_capcache_hash_key()
 *
 * hash function for object references
 *
 * returns hash index 
 */
static int client_capcache_hash_key(const void *key, int table_size)
{
    struct client_capcache_key *real_key = (struct client_capcache_key *) key;
    int tmp_ret = 0;

    tmp_ret = (real_key->refn.handle + real_key->uid) % table_size;
    gossip_debug(GOSSIP_SECURITY_DEBUG, "%s: index: %d H=%llu uid=%d\n",
                 __func__, tmp_ret, llu(real_key->refn.handle), real_key->uid);
    return tmp_ret;
}

/* client_capcache_free_payload()
 *
 * frees payload that has been stored in the client_capcache 
 *
 * returns 0 on success, -PVFS_error on failure
 */
static int client_capcache_free_payload(void *payload)
{
    struct client_capcache_payload *tmp_payload =
         (struct client_capcache_payload *) payload;

    if (tmp_payload == NULL)
    {
        return -PVFS_EINVAL;
    }
  
    PINT_cleanup_capability(&tmp_payload->cap);

    free(tmp_payload);

    return 0;
}

static int set_client_capcache_defaults(struct PINT_tcache *instance)
{
    int ret;

    ret = PINT_tcache_set_info(instance, TCACHE_HARD_LIMIT,
                               CLIENT_CAPCACHE_DEFAULT_HARD_LIMIT);
    if (ret < 0)
    {
        return ret;
    }
    ret = PINT_tcache_set_info(instance, TCACHE_SOFT_LIMIT, 
                               CLIENT_CAPCACHE_DEFAULT_SOFT_LIMIT);
    if (ret < 0)
    {
        return ret;
    }
    ret = PINT_tcache_set_info(instance, TCACHE_RECLAIM_PERCENTAGE,
                               CLIENT_CAPCACHE_DEFAULT_RECLAIM_PERCENTAGE);
    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

