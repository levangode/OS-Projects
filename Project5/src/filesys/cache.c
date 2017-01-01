#include "filesys/cache.h"


void cache_init(void){
	int i;
	for(i=0; i<64; i++){
		my_cache[i].in_use = false;
	}
	lock_init(&cache_lock);

}

void cache_destroy(void){
	int i;
	for(i=0; i<64; i++){
		my_cache[i].in_use = false;
	}
	//Deallocated from stack
}


struct cache_block* chache_get_block(block_sector_t disk_sector_id){
	int i;
	for(i=0; i<64; i++){
		if(my_cache[i].disk_sector_id == disk_sector_id){
			return &my_cache[i];
		}
	}
	return NULL;
}


void cache_put_block(struct cache_block* block){

}


void* cache_read_block(struct cache_block* block);
void* cache_zero_block(struct cache_block* block);	//fill cache block with zeroes, return pointer to data.
void cache_mark_block_dirty(struct cache_block* block);
