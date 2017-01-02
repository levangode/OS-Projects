#include "filesys/cache.h"
#include "filesys/filesys.h"
#include <debug.h>

void cache_init(void){
	int i;
	for(i=0; i<CACHE_SIZE; i++){
		my_cache[i].in_use = false;
	}
	lock_init(&cache_lock);

}


void cache_destroy(void){
	int i;
	for(i=0; i<CACHE_SIZE; i++){
		my_cache[i].in_use = false;
	}
	//Deallocated from stack
}


struct cache_block* cache_get_block(block_sector_t disk_sector_id){
	int i;
	for(i=0; i<CACHE_SIZE; i++){
		if(my_cache[i].disk_sector_id == disk_sector_id){
			return &my_cache[i];
		}
	}
	return NULL;
}


void cache_put_block(struct cache_block* block){

}

//must be called if lock is held by us and the block is in use
void write_block_to_disk(struct cache_block * block){
	ASSERT(block!=NULL && block->in_use);
	if(!block->dirty)return;
	block_write(fs_device,block->disk_sector_id,block->data);
	block->dirty = false;
}


//clock-second chance algorithm
struct cache_block * cache_evit(void){
	size_t i = 0;
	while(true){
		//resetting counter
		if(i == CACHE_SIZE){
			i = 0;
		}
		struct cache_block * cur = &my_cache[i];
		//checking if this block is free tu use
		if(!cur->in_use)
			return cur;
		
		if(!cur->accessed){
			break;
		}else{
			cur->accessed = false;
			i++;
		}
	}
	struct cache_block * to_evict = &my_cache[i];
	if(to_evict->dirty){
		write_block_to_disk(to_evict);
	}
	to_evict->in_use = false;
	return to_evict;
}




void* cache_read_block(struct cache_block* block){
	lock_acquire(&cache_lock);
	struct cache_block* tmp = cache_get_block(block->disk_sector_id);


	lock_release(&cache_lock);
	
}
void* cache_zero_block(struct cache_block* block){
	struct inode_disk* res;

	struct cache_block* tmp = cache_get_block(block->disk_sector_id);
	if(tmp != NULL){
		res = tmp->data;
		//memset ()
	} else {
		//evict any block and use the old data as fresh.
	}
	return res;
}	//fill cache block with zeroes, return pointer to data.


void cache_mark_block_dirty(struct cache_block* block){
	block->dirty=true;
}
