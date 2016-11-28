
#include "threads/malloc.h"
#include "threads/palloc.h"

#include "frame.h"
#include <list.h>


void init_frame_table(void){
	list_init(&frame_list);
}



uint8_t * allocate_frame(enum palloc_flags flags, uint8_t *upage){
	uint8_t* page = palloc_get_page(flags);
	struct frame_entry* tmp_entry = malloc(sizeof(struct frame_entry));
	tmp_entry->kpage = page;
	tmp_entry->upage = upage;
	if(page != NULL){
		list_push_back(&frame_list, &tmp_entry->elem);
	}
	return page;
}

void free_frame(void* page){
	palloc_free_page(page);

}
