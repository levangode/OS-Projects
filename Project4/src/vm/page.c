#include "page.h"
#include <hash.h>
#include <debug.h>
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "frame.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include <string.h>
#include "userprog/syscall.h"






void page_init(struct hash* supplemental_page_table){
	hash_init(supplemental_page_table, page_hash_func, page_less_func, NULL);
}

/* Finds a spt entry in the spt table which corresponds to the given user vitual addres
 * Returns NULL if no such entry exists in the spt table
 */
struct spt_entry* find_page_in_supt(void * uvaddr){
	uvaddr = pg_round_down(uvaddr);
	struct spt_entry temp;
	temp.upage = (uint8_t*)uvaddr;
	struct hash_elem * res = hash_find(&thread_current()->supplemental_page_table,&temp.elem);
	if(res != NULL){
		struct spt_entry* val = hash_entry(res,struct spt_entry,elem);
		return val;
	}
	return NULL;
}
void spte_fn (struct hash_elem* to_delete, void* aux){
	struct spt_entry* next = hash_entry(to_delete, struct spt_entry, elem);
	free(next);
}

void free_spt(struct hash* supplemental_page_table){
	hash_destroy(supplemental_page_table, spte_fn);
}


/* Hash function for hashing spt entries by  user virtual addresses */
unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED){
	struct spt_entry* entry = hash_entry(e, struct spt_entry, elem);
  	return hash_int((int)entry->upage);
}


/* Compare function for comparing two spt entries by their user virtual addresses */
bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux UNUSED){

	struct spt_entry* as = hash_entry(a, struct spt_entry, elem);
  	struct spt_entry* bs = hash_entry(b, struct spt_entry, elem);
	return as->upage < bs->upage;
}


/* Adds new spt entry to hash. */
void spt_add(uint8_t* upage, uint8_t* kpage, bool writable, bool loaded){
	struct spt_entry* tmp_entry = malloc(sizeof(struct spt_entry));
	tmp_entry->upage = upage;
	tmp_entry->kpage = kpage;
	tmp_entry->writable = writable;
	if(loaded){
		tmp_entry->loaded = true;
	}

	hash_insert(&thread_current()->supplemental_page_table, &tmp_entry->elem);
}



/* Grows the stack by allocating new spt table entry and corresponding user frame */
bool stack_growth(uint8_t* uvaddr){
	if((uint8_t*)PHYS_BASE - uvaddr > STACK_LIMIT){
    return false;
  }
  struct spt_entry* tmp_entry = malloc(sizeof(struct spt_entry));
 	uint8_t* upage = uvaddr;
 	bool writable = true;

 	tmp_entry->upage = uvaddr;
 	tmp_entry->page_type = 1;
 	tmp_entry->writable = true;
  hash_insert(&thread_current()->supplemental_page_table, &tmp_entry->elem);
  return load_page(upage);
}


bool load_page(uint8_t* upage){
	upage = pg_round_down(upage);
	bool res = true;
	struct spt_entry* tmp_entry = find_page_in_supt(upage);
	if(tmp_entry == NULL){
		return false;
	}

	/*if(tmp_entry->loaded){
		printf("%s\n", "###############################");
		return true;
	}*/
	void* kpage = allocate_frame(PAL_USER, upage);
	if(kpage == NULL){
		return false;
	}

	if(tmp_entry->page_type == FROM_FILE){	//FILE

		struct file* load_file = tmp_entry->f;
		int offset = tmp_entry->offset;
		int bytes_read = tmp_entry->bytes_read;
		int bytes_zero = tmp_entry->bytes_zero;
		bool release = false;
		if(!lock_held_by_current_thread(&system_global_lock)){
			lock_acquire(&system_global_lock);
			release = true;
		}
		file_seek(load_file, offset);

		if (file_read (load_file, kpage, bytes_read) != (int) bytes_read)
        {
          palloc_free_page (kpage);
          if(release)
          	lock_release(&system_global_lock);
          return false; 
        }
        if(release)
        	lock_release(&system_global_lock);
        memset ((char*)kpage + bytes_read, 0, bytes_zero);
        res = true;


	} else if(tmp_entry->page_type == ALL_ZERO){ 	//ZEROPAGE
		res = true;
	} else if(tmp_entry->page_type == FROM_SWAP){	//SWAP

	} else if(tmp_entry->page_type == FROM_MMAP){	//MMAP
		//PANIC("asd");
		struct file* load_file = tmp_entry->f;
		int offset = tmp_entry->offset;
		int bytes_read = tmp_entry->bytes_read;
		int bytes_zero = tmp_entry->bytes_zero;
		bool release = false;
		if(!lock_held_by_current_thread(&system_global_lock)){
			lock_acquire(&system_global_lock);
			release = true;
		}
		file_seek(load_file, offset);

		if (file_read (load_file, kpage, bytes_read) != (int) bytes_read)
        {
          palloc_free_page (kpage);
          if(release)
          	lock_release(&system_global_lock);
          return false; 
        }
        if(release)
        	lock_release(&system_global_lock);
        memset ((char*)kpage + bytes_read, 0, bytes_zero);
        res = true;

		//PANIC("%c%c%c%c%c", (char*)upage, (char*)upage+1, (char*)upage+2, (char*)upage+3, (char*)upage+4);
        //printf("asdasdasdas\n" );
		//printf("asdasdasdasdasd1111111111\n");	
		//PANIC("ACCESS MAPPED FILE %d", res);

	} else {
		PANIC("SHOULD HAVE ENTERED ANY OF THE CASES");
		return false;
	}
	if (!install_page(upage, kpage, tmp_entry->writable)){
  	palloc_free_page (kpage);
    return false; 
  }

  tmp_entry->loaded = true;
  return true;
}


void pin_page(void* page){
	struct spt_entry* element = find_page_in_supt(page);
	if(element == NULL)return;
	uint8_t* kpage = element->kpage;
	struct frame_entry* curElem = find_frame(kpage);
	lock_acquire(&list_lock);
	curElem->is_pinned = true;
	lock_release(&list_lock);
	
}

void unpin_page(void* page){
	struct spt_entry* element = find_page_in_supt(page);
	if(element == NULL)return;
	uint8_t* kpage = element->kpage;
	struct frame_entry* curElem = find_frame(kpage);
	lock_acquire(&list_lock);
	curElem->is_pinned = true;
	lock_release(&list_lock);
}




void make_spt_dirty(void* page, bool dirty){
	struct hash spt_table = thread_current()->supplemental_page_table;
	struct spt_entry* element = find_page_in_supt(page);
	if(element == NULL){
		PANIC("ATTEMPTED TO SET DIRTY BUT PAGE WASN'T FOUND");
	}
	if(dirty){
		element->isDirty = true;
	}
	return true;
}






bool spt_install_file(void* upage,struct file* f,off_t offset, size_t bytes_read, size_t bytes_zero, bool writable){
	struct hash spt_page_table = thread_current()->supplemental_page_table;
	struct spt_entry * new_spt_entry = malloc(sizeof(struct spt_entry));
	if(new_spt_entry == NULL){
		return false;
	}
	new_spt_entry->page_type = FROM_FILE;
	new_spt_entry->isDirty = false;
	new_spt_entry->f = f;
	new_spt_entry->offset = offset;
	new_spt_entry->upage = upage;
	new_spt_entry->kpage = NULL;
	new_spt_entry->bytes_zero = bytes_zero;
	new_spt_entry->bytes_read = bytes_read;
	new_spt_entry->writable = writable;
	new_spt_entry->loaded = false;

	struct hash_elem * element = hash_insert(&spt_page_table,&new_spt_entry->elem);
	if(element == NULL){
		return true;
	}else{
		PANIC("DUPLICATE SPT ENTRY!");
		return false;
	}	
}

/*
	installs page for mapped file
*/
bool spt_install_file_mmap(void* upage,struct file* f,off_t offset, size_t bytes_read, size_t bytes_zero, bool writable){
	struct hash spt_page_table = thread_current()->supplemental_page_table;
	struct spt_entry * new_spt_entry = malloc(sizeof(struct spt_entry));
	if(new_spt_entry == NULL){
		return false;
	}
	new_spt_entry->page_type = FROM_MMAP;
	new_spt_entry->isDirty = false;
	new_spt_entry->f = f;
	new_spt_entry->offset = offset;
	new_spt_entry->upage = upage;
	new_spt_entry->kpage = NULL;
	new_spt_entry->bytes_zero = bytes_zero;
	new_spt_entry->bytes_read = bytes_read;
	new_spt_entry->writable = writable;

	struct hash_elem * element = hash_insert(&spt_page_table,&new_spt_entry->elem);
	if(element == NULL){
		return true;
	}else{
		PANIC("DUPLICATE SPT ENTRY!");
		return false;
	}	
}
