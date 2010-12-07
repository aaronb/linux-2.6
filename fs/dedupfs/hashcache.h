// hashcache.h
// This module implements a fixed size cache that maps data block hash
// values to block numbers.
//
// Implementation:
// Under the hood a 16 bit hash table is used for storing the entries.
// 16 bits of the hash are used as a table index and the remaining portion of the hash
// value is stored with each entry.  Thus to look up an entry given a hash,
// you first split it up, index into the hash table, then scan through the items
// in the bucket until an item is found that matches the remainder hash bits.
//
// Replacement Policy:
// When inserting an item when the cache is full, an item will be evicted
// according to a quasi clock algorithm.  The pointer is advanced to a
// non-empty bucket, and the first item in the bucket (the oldest in the
// bucket) is evicted.
//

#ifndef _HASHCACHE_H
#define _HASHCACHE_H

#include <linux/slab.h>
#include <linux/string.h>

#define HT_IBITS (16)
#define HT_SIZE (1 << HT_IBITS)
#define BYTES(bits) (bits/8) // 8 bits per byte?

typedef uint32_t block_ptr_t;
typedef uint32_t ht_index_t;
typedef char tag_t;

typedef struct ht_item_t__ {
    tag_t *tag;                 // pointer to a char array containing the tag
    block_ptr_t blknum;         // block number associated with this entry
    struct ht_item_t__ * next;  // pointer to next item
} ht_item_t;

typedef struct hash_cache_t__ {
    size_t hash_len;           // length of the hashvalue in bytes
    size_t idx_len;            // num bytes of the hashvalue used to index table
    size_t tag_len;            // num bytes of the hashvalue stored with item entry
    size_t max_items;          // max num items that can be stored given the cache size
    size_t ht_size;            // number of buckets in hash table
    ht_index_t evict_bucket;   // clock hand used when evicting items
    ht_item_t **table;         // an array of pointers to the buckets
    ht_item_t *itempool;       // the pool of memory in which the item entries are stored
    char *tagpool;             // memory pool that holds the tags
    ht_item_t *free_items;     // head of the linked list of free item slots
} hash_cache_t;

// public functions

int hashcache_init(hash_cache_t *hc, size_t cache_size, size_t hashlen);
int hashcache_destroy(hash_cache_t *hc);
int hashcache_get(hash_cache_t *hc, char *hashval, block_ptr_t *blknum);
int hashcache_insert(hash_cache_t *hc, char *hashval, block_ptr_t blknum);
int hashcache_update(hash_cache_t *hc, char *hashval, block_ptr_t blknum);
int hashcache_remove(hash_cache_t *hc, char *hashval);

// private, user shouldn't need to use these

int hashcache_split_hashval(hash_cache_t *hc, char *hashval, char *tag, ht_index_t *idx);
void hashcache_evict(hash_cache_t *hc);

#endif
