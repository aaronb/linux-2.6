#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "hashcache.h"


void do_init(hash_cache_t *hc, size_t cache_size, size_t hashlen) {
    if (hashcache_init(hc, cache_size, hashlen) != 0) {
        printf("Hashcache Init failed\n");
        exit(-1);
    }
}

void do_insert(hash_cache_t *hc, char *hash, block_ptr_t blknum) {
    if (hashcache_insert(hc, hash, blknum) != 0) {
        printf("Insert failed %.*s (%d)\n", (int)hc->hash_len, hash, (int)blknum);
    } else {
        printf("Inserted: %.*s (%d)\n", (int)hc->hash_len, hash, (int)blknum);
    }
}

void do_lookup(hash_cache_t *hc, char *hash) {
    block_ptr_t blknum = 0;
    if (hashcache_get(hc, hash, &blknum) != 0) {
        printf("Lookup failed: %.*s\n", (int)hc->hash_len, hash);
    } else {
        printf("Lookup found: %.*s (%d)\n", (int)hc->hash_len, hash, blknum);
    }
}

void do_remove(hash_cache_t *hc, char *hashval) {
    if (hashcache_remove(hc, hashval) != 0) {
        printf("Remove failed: %.*s\n", (int)hc->hash_len, hashval);
    } else {
        printf("Removed: %.*s\n", (int)hc->hash_len, hashval);
    }
}

void do_update(hash_cache_t *hc, char *hashval, block_ptr_t blknum) {
    if (hashcache_update(hc, hashval, blknum) != 0) {
        printf("Update failed: %.*s (%d)\n", (int)hc->hash_len, hashval, (int)blknum);
    } else {
        printf("Updated: %.*s (%d)\n", (int)hc->hash_len, hashval, (int)blknum);
    }
}



int main (int argc, char const *argv[]) {
    int i;
    hash_cache_t hc;
    size_t hash_len = 256 / 8;
    size_t cache_size = 1 << 19;
    
    char hash1[hash_len];
    char hash2[hash_len];
    char hash3[hash_len];
    for (i=0; i<hash_len; i++) {
        hash1[i] = 'a';
        hash2[i] = (i == 0) ? 'b' : 'a';
        hash3[i] = (i == 0) ? 'c' : 'a';
    }
    
    
    printf("Test: basic inserts and lookups\n");
    do_init(&hc, cache_size, hash_len);
    sleep(60);
    do_insert(&hc, hash1, 111);
    do_insert(&hc, hash2, 222);
    do_insert(&hc, hash3, 333);
    do_lookup(&hc, hash1);
    do_lookup(&hc, hash2);
    do_lookup(&hc, hash3);
    hashcache_destroy(&hc);
    printf("\n");
    
    
    printf("Test: removing an entry\n");
    do_init(&hc, cache_size, hash_len);
    do_insert(&hc, hash1, 111);
    do_lookup(&hc, hash1);
    do_remove(&hc, hash1);
    do_lookup(&hc, hash1);
    hashcache_destroy(&hc);
    printf("\n");
    
    
    printf("Test: updating a block number for a hash\n");
    do_init(&hc, cache_size, hash_len);
    do_insert(&hc, hash1, 111);
    do_lookup(&hc, hash1);
    do_update(&hc, hash1, 111111);
    do_lookup(&hc, hash1);
    hashcache_destroy(&hc);
    printf("\n");
    
    
    printf("Test: inserting when cache is full should evict an entry\n");
    do_init(&hc, cache_size, hash_len);
    char h[hash_len];
    for (i=0; i<hash_len; i++) h[i] = 'a';
    for (i=0; i<=hc.max_items; i++) {
        *(int*)h = i; // update the hash to make it unique
        if (hashcache_insert(&hc, h, i) != 0) break;
    }
    printf("Eviction bucket: %d\n", hc.evict_bucket);
    hashcache_destroy(&hc);
    printf("\n");
    
    
    return 0;
}


