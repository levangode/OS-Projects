
#include "threads/malloc.h"
#include "threads/palloc.h"

#include "frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

void init_frame_table(void){
	list_init(&frame_list);
	lock_init(&list_lock);
}


struct frame_entry* find_frame(uint8_t* kpage){
	lock_acquire(&list_lock);
	struct list_elem * tmp;
	for(tmp = list_begin(&frame_list); tmp != list_end(&frame_list); tmp = list_next(tmp) ){
		struct frame_entry * curElem =  (struct frame_entry * ) list_entry(tmp,struct frame_entry,elem);
		if(curElem->kpage == kpage){
			lock_release(&list_lock);
			return curElem;
		}
	}
	lock_release(&list_lock);
	return NULL;
}


uint8_t * allocate_frame(enum palloc_flags flags, uint8_t *upage){
	uint8_t* page = palloc_get_page(flags);
	if(page == NULL){
		lock_acquire(&list_lock);
		eviction(upage,flags);
		page = palloc_get_page(flags);
		lock_release(&list_lock);
	}
	struct frame_entry* tmp_entry = malloc(sizeof(struct frame_entry));
	tmp_entry->kpage = page;
	tmp_entry->upage = upage;
	tmp_entry->is_pinned = true;
	if(page != NULL){
		lock_acquire(&list_lock);
		list_push_back(&frame_list, &tmp_entry->elem);
		lock_release(&list_lock);
	}
	return page;
}

bool check_dirty(struct frame_entry* evicted){
	if(pagedir_is_dirty(evicted->occupying_thread->pagedir,evicted->upage) ||pagedir_is_dirty(evicted->occupying_thread->pagedir,evicted->kpage)){
		return true;
	}
	return false;
}


//used algorithm described on seminar
void eviction(uint8_t *upage,enum palloc_flags flags){
	if(list_size(&frame_list) ==0){
		PANIC("FRAME LIST IS EMPTY");
	}
	size_t size = list_size(&frame_list);
	size_t counter = 0;
	struct frame_entry* evicted = NULL;
	struct list_elem* temp = NULL;
	while(true){
		if(counter == ITERATION_NUM * size)break;
		if(temp == NULL || temp == list_end(&frame_list)){
			temp = list_begin(&frame_list);
		}else{
			temp = list_next(temp);
		}
		struct frame_entry* curElem = (struct frame_entry * ) list_entry(temp,struct frame_entry,elem);
		if(curElem->is_pinned)continue;
		struct thread* curThread = (struct thread*)thread_current();
		if(!pagedir_is_accessed(curThread->pagedir,curElem->upage)){
			evicted = curElem;
			break;
		}else{
			pagedir_set_accessed(curThread->pagedir,curElem->upage,false);
		}
		counter++;
	}
	if(evicted == NULL){
		PANIC("NOTHING EVICTED. GET MORE RAM");
	}
	if(evicted->occupying_thread == NULL){
		PANIC("EVICTED ELEMENT'S THREAD INFO MISSING");
	}
	//need to check evicted element's occupying thread's pagedir..thinking about how to do that
	pagedir_clear_page(evicted->occupying_thread->pagedir,evicted->upage);
	//next steps require swap..waiting for it
	make_spt_swap(evicted->upage,swap_out(evicted->kpage));
	make_spt_dirty(evicted->upage,check_dirty(evicted));
	struct frame_entry *frame = find_frame(evicted->kpage);
	list_remove(&frame->elem);
	free_frame(evicted->kpage);
	free(frame);
}



void free_frame(void* page){
	palloc_free_page(page);
}
