#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H
#include <stdbool.h>
#include "threads/synch.h"
#include "devices/block.h"

#define CACHE_SIZE 64

struct cache_block{
	uint8_t data[BLOCK_SECTOR_SIZE];
	block_sector_t disk_sector_id;
	bool dirty;
	bool accessed;
	bool in_use;
};

void fill_block_info_after_eviction(struct cache_block* res,block_sector_t sector_id, bool should_read);
struct cache_block my_cache[CACHE_SIZE];	//cache sectors size limit
struct cache_block * cache_evit(void);
struct lock cache_lock;
void write_cache_to_disk(void);
void cache_init(void);
void cache_destroy(void);
struct cache_block* chache_get_block(block_sector_t disk_sector_id);
void write_block_to_disk(struct cache_block * block);
void cache_put_block(struct cache_block* block);
void* cache_read_block(struct cache_block* block);
void* cache_zero_block(struct cache_block* block);	//fill cache block with zeroes, return pointer to data.



void cache_mark_block_dirty(struct cache_block* block);
#endif