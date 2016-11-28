#ifndef _FRAME_
#define _FRAME_

#include <list.h>
#include "threads/synch.h"

struct list frame_list;
struct lock list_lock;

struct frame_entry{
	uint8_t* kpage;
	uint8_t* upage;
	struct list_elem elem;
};


void init_frame_table(void);
uint8_t * allocate_frame(enum palloc_flags flags, uint8_t *upage);
void free_frame(void* page);




#endif