
#include "threads/malloc.h"
#include "threads/palloc.h"

#include "frame.h"
#include <list.h>
#include "threads/synch.h"

void init_frame_table(void){
	list_init(&frame_list);
	lock_init(&list_lock);
}



uint8_t * allocate_frame(enum palloc_flags flags, uint8_t *upage){
	uint8_t* page = palloc_get_page(flags);
	struct frame_entry* tmp_entry = malloc(sizeof(struct frame_entry));
	tmp_entry->kpage = page;
	tmp_entry->upage = upage;
	if(page != NULL){
		lock_acquire(&list_lock);
		list_push_back(&frame_list, &tmp_entry->elem);
		lock_release(&list_lock);
	}
	return page;
}
//used algorithm described on seminar
void * eviction(uint8_t *upage,enum palloc_flags flags){
	if(hash_size(&frame_list) ==0){
		PANIC("FRAME LIST IS EMPTY");
	}
	size_t size = hash_size(&frame_list);
	size_t counter = 0;
	struct frame_entry* evicted = NULL;
	while(true){
		if(counter == ITERATION_NUM * size)break;
		//TODO pick evitable one

		counter++;
	}
	if(evicted == NULL){
		PANIC("NOTHING EVICTED. GET MORE RAM");
	}

}



void free_frame(void* page){
	palloc_free_page(page);

}
