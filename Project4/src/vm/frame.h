#ifndef _FRAME_
#define _FRAME_

#include <list.h>


struct list frame_list;


struct frame_entry{
	uint8_t* page;
	struct list_elem elem;
};


void init_frame_table(void);
uint8_t * allocate_frame(enum palloc_flags flags);
void free_frame(void* page);




#endif