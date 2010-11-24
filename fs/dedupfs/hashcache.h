// hashcache.h
// 
// An interface for storing a mapping from the hash value of
// data blocks to blocknumber that contain data witht the same
// hashvalue.
//

#define IDEAL_HT_LOAD (0.7)

struct hash_cache {
  void *table;
  void *pool;
  size_t hashbits_in_record;
  size_t hashbits_in_index;
};


// Initialize the hash cache structures.
void dedup_hash_init(struct super_block *sb);

// Lookup a list of block numbers that have a given hashvalue
void dedup_hash_lookup(void* hashval,
                       __le32 target_block,
                       __le32 * matched_blocks,
                       size_t n);

// Removes a hashval/blocknum pair from the hash cache
void dedup_hash_remove(void* hashval,
                       __le32 block);


