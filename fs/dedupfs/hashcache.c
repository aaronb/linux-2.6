// Initialize the hash cache structures.
// hash_size: number of bits in the hash value
// memory_bound: max size in bytes to allocate for the cache

// Initialize the hash cache structures.
void dedup_hash_init(struct super_block *sb,
                     size_t hash_len)
{
  dedupfs_sb_info *sbi = DEDUPFS_SB(sb);
  
  int blocknum_size = sizeof(__le32);
  int pointer_size = sizeof(void*);
  int n_buckets, n_entries;
  int index_bits, entry_bits;
  int entry_size;
  float occupancy;  
  
  for (int i=0; i<hash_len; i++) {
    entry_bits = i;
    index_bits = hash_len - entry_bits;
    int rounded_entry_bits = entry_bits / 8 + 8
    entry_size = rounded_entry_bits + blocknum_size + pointer_size;
    n_entries = sbi->hash_cache_size / entry_size;
    n_buckets = 1 << (index_bytes * 8);
    occupancy = (float)n_entries / (float)n_buckets;
    if (occupancy > 0.75) break;
  }
  
  sbi->hash_len = hash_len;
  sbi->hash_cache_size = entry_size * n_entries;

  struct hash_cache *hc = kmalloc(sizeof(struct hash_cache));
  hc->table = kmalloc(n_buckets * sizeof(void*), GFP_KERNEL);
  hc->pool = kmalloc(hc->hash_cache_size, GFP_KERNEL);
  
  sbi->hash_cache = hashcache
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


