#include <limits.h>
#include "dedupfs.h"
#include "hashcache.h"

#define IDEAL_HT_LOAD (0.7)

struct hash_cache {
  void *table;
  void *pool;
  size_t hashbits_in_record;
  size_t hashbits_in_index;
};

// Initialize the hash cache structures.
void dedup_hash_init(struct super_block *sb)
{
  dedupfs_sb_info *sbi = DEDUPFS_SB(sb);
  
  size_t blknum_size = sizeof(__le32);
  size_t ptr_size = sizeof(void*);
  size_t hash_bits = sbi->hash_len * CHAR_BIT;
  
  size_t n_buckets, n_records;
  size_t rbits, ibits;
  size_t record_size;
  
  // Iterating over number of bytes of the hashval to store in each record
  for (int i=0; i <= sbi->hash_len; i++) {
    rbits = i * CHAR_BIT; // num bits to store in record
    
    // size of record in bytes
    record_size = i + blknum_size + ptr_size;
    
    // numb records at this size that can fit in the hash cache space
    n_records = sbi->hash_cache_size / record_size;
    
    // ideal number of buckets for n_records
    n_buckets = (int)((float)n_records / IDEAL_HT_LOAD);
    
    // calculate num bits needed to index a table with n_buckets
    for (ibits=0; ibits < hash_bits; ibits++) {
      if ((1 << ibits) > n_buckets) break;
    }

    // if the ibits needed is greater than the remainder bits after
    // stripping off the record bits, then we found a good combo
    if (ibits > (hash_bits - ): break;
  }
  
  struct hash_cache *hc = kmalloc(sizeof(struct hash_cache), GFP_KERNEL);
  hc->table = kmalloc(n_buckets * ptr_size, GFP_KERNEL);
  hc->pool = kmalloc(hc->hash_cache_size, GFP_KERNEL);
  hc->hashbits_in_record = rbits;
  hc->hashbits_in_index = ibits;
  
  sbi->hash_cache = hashcache;
}

// Lookup a list of block numbers that have a given hashvalue
void dedup_hash_lookup(void* hashval,
                       __le32 target_block,
                       __le32 * matched_blocks,
                       size_t n)
{
  // TODO
}

// Removes a hashval/blocknum pair from the hash cache
void dedup_hash_remove(void* hashval,
                       __le32 block)
{
  // TODO
}


