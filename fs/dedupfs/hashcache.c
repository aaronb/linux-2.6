#include "hashcache.h"
#include <linux/mutex.h>
#include <linux/string.h>

DEFINE_MUTEX(lock);

int hashcache_split_hashval(hash_cache_t *hc, char *hashval, char *tag, ht_index_t *idx) {
    // strip out the item bytes
    memcpy(tag, hashval, hc->tag_len);
    
    // strip out the hash table index from end
    *idx = (ht_index_t)hashval[hc->tag_len]; 
    
    return 0;
}

int hashcache_init(hash_cache_t *hc, size_t cache_size, size_t hashlen) {
    size_t item_size;
    int i;
    ht_item_t *item;

    hc->hash_len = hashlen;
    hc->idx_len = BYTES(HT_IBITS);
    hc->tag_len = hashlen - hc->idx_len;
    
    item_size = sizeof(ht_item_t) + hc->tag_len;
    hc->max_items = cache_size / item_size;
    hc->ht_size = HT_SIZE;
    
    // the cache must be large enough to hold at least 8 items
    // this is an arbitrary choice, but there's no point to hold less than this
    // you'd just miss most of the time.
    if (hc->max_items < 8) return -1;
    
    hc->table = kmalloc(hc->ht_size * sizeof(ht_item_t*), GFP_KERNEL);
    if (hc->table == NULL) return -1;
    
    hc->itempool = kmalloc(hc->max_items * sizeof(ht_item_t), GFP_KERNEL);
    if (hc->itempool == NULL) return -1;
    
    hc->tagpool = kmalloc(hc->max_items * hc->tag_len, GFP_KERNEL);
    if (hc->tagpool == NULL) return -1;
    
    hc->free_items = hc->itempool;
    
    // initialize each item slot with dummy data
    // link them all into the free list
    item = NULL;
    for (i=0; i<hc->max_items; i++) {
        item = &(hc->itempool[i]);
        item->tag = hc->tagpool + i * hc->tag_len;
        memset(item->tag, 'x', hc->tag_len);
        item->blknum = i;
        item->next = (i == hc->max_items - 1) ? NULL : &(hc->itempool[i+1]);
    }
    
    hc->evict_bucket = 0;

	mutex_init(&lock);
    
    return 0;
}

int hashcache_destroy(hash_cache_t *hc) {
    kfree(hc->table);
    kfree(hc->itempool);
    kfree(hc->tagpool);
    return 0;
}


int hashcache_get(hash_cache_t *hc, char *hashval, block_ptr_t *blknum) {
    char tag[hc->tag_len];
    ht_index_t idx;
    ht_item_t *item;

    hashcache_split_hashval(hc, hashval, tag, &idx);
    
    item = NULL;
    if ((item = hc->table[idx]) == NULL) return -1; // bucket empty
    
    while (item != NULL) {
        if (memcmp(item->tag, tag, hc->tag_len) == 0) {
            *blknum = item->blknum;
            return 0;
        }
        item = item->next;
    }
    
    return -1;
}

int hashcache_insert(hash_cache_t *hc, char *hashval, block_ptr_t blknum) {
    ht_index_t idx;
    char tag[hc->tag_len];
    ht_item_t *prev, *item, *new_item;

    hashcache_split_hashval(hc, hashval, tag, &idx);
    
    prev = NULL;
    item = hc->table[idx];
    while (item != NULL) {
        // existing item with this hash value?
        if (memcmp(item->tag, tag, hc->tag_len) == 0) {
		mutex_unlock(&lock);
            return -1; // duplicate found
        }
        prev = item;
        item = item->next;
    }    

    mutex_lock(&lock);

    // if there are no free items, we need to evict something
    if (hc->free_items == NULL) hashcache_evict(hc);

    if(hc->free_items == NULL) {
	    //printk(KERN_ERR "hc->free_items null after eviction\n");
	    mutex_unlock(&lock);
	    return -1;
    }


    // insertion point found, need to grab a new item from the pool
    new_item = hc->free_items;
    hc->free_items = hc->free_items->next;
    
    new_item->next = NULL;
    new_item->blknum = blknum;
    memcpy(new_item->tag, tag, hc->tag_len);
    
    // link new item into the bucket
    if (prev == NULL) {
        // bucket was empty and we're adding the first item
        hc->table[idx] = new_item;
    } else {
        prev->next = new_item;
    }

    mutex_unlock(&lock);
    
    return 0;
}

int hashcache_update(hash_cache_t *hc, char *hashval, block_ptr_t blknum) {
    ht_index_t idx;
    ht_item_t *prev, *item, *new_item;
    char tag[hc->tag_len];

    hashcache_split_hashval(hc, hashval, tag, &idx);
    
    prev = NULL;
    item = hc->table[idx];
    while (item != NULL) {
        if (memcmp(item->tag, tag, hc->tag_len) == 0) {
            // found existing item with this hash value
            item->blknum = blknum;
            return 0;
        }
        prev = item;
        item = item->next;
    }
    
    // no match was found to update so we need to grab a new item from the pool
    if (hc->free_items == NULL) return -1;
    new_item = hc->free_items;
    hc->free_items = hc->free_items->next;
    
    // initialize the new item
    new_item->next = NULL;
    new_item->blknum = blknum;
    memcpy(new_item->tag, tag, hc->tag_len);
    
    // link new item into the bucket
    if (prev == NULL) { // bucket was empty and we're adding the first item
        hc->table[idx] = new_item;
    } else {
        prev->next = new_item;
    }
    
    return 0;
}

int hashcache_remove(hash_cache_t *hc, char *hashval) {
    ht_index_t idx;
    char tag[hc->tag_len];
    ht_item_t *prev, *item;

    hashcache_split_hashval(hc, hashval, tag, &idx);
    
    prev = NULL;
    item = hc->table[idx];
    while (item != NULL) {
        if (memcmp(item->tag, tag, hc->tag_len) == 0) {
            // found existing item with this hash value
            // remove from bucket list
            if (prev == NULL) hc->table[idx] = NULL;
            else prev->next = item->next;
            
            // blank out the item, and add to free list
            item->next = hc->free_items;
            item->blknum = 0;
            memset(item->tag, 'x', hc->tag_len);
            hc->free_items = item;
            
            return 0;
        }
        prev = item;
        item = item->next;
    }
    
    // no match found
    return -1;
}

void hashcache_evict(hash_cache_t *hc) {
    // printf("Evicting an item\n");
    ht_item_t *item = NULL;
    int i;
    for (i=0; i<hc->ht_size; i++) {
        hc->evict_bucket = (hc->evict_bucket + 1) % hc->ht_size;
        
        if ((item = hc->table[hc->evict_bucket]) != NULL) {
            hc->table[hc->evict_bucket] = item->next;
            item->next = hc->free_items;
            item->blknum = 0;
            memset(item->tag, 'x', hc->tag_len);
            hc->free_items = item;
            return;
        }
    }

    if(hc->free_items == NULL) {
	    printk(KERN_ERR "hc->free_items null after eviction\n");
	    return ;
    }
}


