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
void swap_in (int index, void *swap_pg){
lock_acquire(&swaplock);
size_t i;
for(i=0;i < PGSIZE / BLOCK_SECTOR_SIZE;i++ ){
	block_read(swapblock,index * (PGSIZE / BLOCK_SECTOR_SIZE) + i,swap_pg + i * BLOCK_SECTOR_SIZE );
}
bitmap_set(swapblock, index, 1);
lock_release(&swaplock);
}