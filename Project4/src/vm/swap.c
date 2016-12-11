#include "swap.h"
#include "threads/vaddr.h"
#include "devices/block.h"
#include <bitmap.h>
#include "threads/synch.h"
struct block *swapblock;
struct bitmap *swapmap;
struct lock swaplock;
void swap_init (void){
	lock_init(&swaplock);
	swapblock = block_get_role(BLOCK_SWAP);
	swapmap = bitmap_create(block_size(swapblock) / (PGSIZE / BLOCK_SECTOR_SIZE));
	bitmap_set_all(swapmap,1);
}
void swap_in (size_t index, void *swap_pg){
lock_acquire(&swaplock);
size_t i;
for(i=0;i < PGSIZE / BLOCK_SECTOR_SIZE;i++ ){
	block_read(swapblock,index * (PGSIZE / BLOCK_SECTOR_SIZE) + i,swap_pg + i * BLOCK_SECTOR_SIZE );
}
bitmap_set(swapblock, index, 1);
lock_release(&swaplock);
}
size_t swap_out (void *swap_pg){
	lock_acquire(&swaplock);
	size_t index = bitmap_scan(swapmap,0,1,1);
	size_t i;
	for(i=0;i < PGSIZE / BLOCK_SECTOR_SIZE;i++){
		block_write(swapblock,index * (PGSIZE / BLOCK_SECTOR_SIZE) + i,swap_pg + (BLOCK_SECTOR_SIZE * i));
	}
	bitmap_set(swapmap, index, 0);
	lock_release(&swaplock);
	return index;
}
void swap_free (size_t index){
	lock_acquire(&swaplock);
	bitmap_set(swapblock,index,1);
	lock_release(&swaplock);
}
