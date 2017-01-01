#include <stdbool.h>
#include "threads/synch.h"
#include "devices/block.h"

struct cache_block{
	block_sector_t disk_sector_id;
	bool dirty;
	bool accessed;
	bool in_use;
};

struct cache_block my_cache[64];	//cache sectors size limit

struct lock cache_lock;

void cache_init(void);
void cache_destroy(void);
struct cache_block* chache_get_block(block_sector_t disk_sector_id);
void cache_put_block(struct cache_block* block);
void* cache_read_block(struct cache_block* block);
void* cache_zero_block(struct cache_block* block);	//fill cache block with zeroes, return pointer to data.
void cache_mark_block_dirty(struct cache_block* block);
