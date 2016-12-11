#ifndef _FRAME_
#define _FRAME_

#include <list.h>
#include "threads/synch.h"
#define ITERATION_NUM 4
struct list frame_list;
struct lock list_lock;

struct frame_entry{
	uint8_t* kpage;
	uint8_t* upage;
	bool is_pinned; //if true this frame can't be evicted
	struct thread* occupying_thread; //needed to prevent thread from evicting it's own memory
	struct list_elem elem;
};


void init_frame_table(void);
uint8_t * allocate_frame(enum palloc_flags flags, uint8_t *upage);
void free_frame(void* page);
struct frame_entry* find_frame(uint8_t* kpage);
void * eviction(uint8_t *upage,enum palloc_flags flags);




#endif