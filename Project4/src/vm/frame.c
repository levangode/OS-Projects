
#include "threads/malloc.h"
#include "threads/palloc.h"

#include "frame.h"
#include <list.h>


void init_frame_table(void){
	list_init(&frame_list);
}



uint8_t * allocate_frame(enum palloc_flags flags){
	uint8_t* page = palloc_get_page(flags);
	struct frame_entry* tmp_entry = malloc(sizeof(struct frame_entry));
	tmp_entry->page = page;
	if(page != NULL){
		list_push_back(&frame_list, &tmp_entry->elem);
	}
	return page;
}
