#include "page.h"
#include <hash.h>
#include <debug.h>
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "frame.h"
#include "threads/vaddr.h"
#include "userprog/process.h"



#define STACK_LIMIT 0x800000  // == 2^23


void page_init(struct hash* supplemental_page_table){
	hash_init(supplemental_page_table, page_hash_func, page_less_func, NULL);
}

/* Finds a spt entry in the spt table which corresponds to the given user vitual addres
 * Returns NULL if no such entry exists in the spt table
 */
struct spt_entry* find_page_in_supt(void * uvaddr){
	struct spt_entry temp;
	temp.upage = (uint8_t*)uvaddr;
	struct hash_elem * res = hash_find(&thread_current()->supplemental_page_table,&temp.elem);
	if(res != NULL){
		struct spt_entry* val = hash_entry(res,struct spt_entry,elem);
		return val;
	}
	return NULL;
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
void spt_add(uint8_t* upage, uint8_t* kpage, bool writable){
	struct spt_entry* tmp_entry = malloc(sizeof(struct spt_entry));
	tmp_entry->upage = upage;
	tmp_entry->kpage = kpage;
	tmp_entry->writable = writable;

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
	bool res = true;
	struct spt_entry* tmp_entry = find_page_in_supt(upage);
	if(tmp_entry == NULL){
		return false;
	}
	void* kpage = allocate_frame(PAL_USER, upage);
	if(kpage == NULL){
		return false;
	}
	if(tmp_entry->page_type == FROM_FILE){	//FILE
		//get file
	} else if(tmp_entry->page_type == ALL_ZERO){ 	//ZEROPAGE
		res = true;
	} else if(tmp_entry->page_type == FROM_SWAP){	//SWAP

	} else if(tmp_entry->page_type == FROM_MMAP){	//MMAP

	} else {
		PANIC("SHOULD HAVE ENTERED ANY OF THE CASES");
		return false;
	}
	if (!install_page (upage, kpage, true))	{
  		palloc_free_page (kpage);
    	return false; 
    }
    return res;
}

bool spt_install_file(void* upage,struct file* f,off_t offset,unsigned int bytes_read,unsigned int bytes_zero,bool writable){
	struct hash spt_page_table = thread_current()->supplemental_page_table;
	struct spt_entry * new_spt_entry = (struct spt_entry*)malloc(sizeof(struct spt_entry));
	new_spt_entry->page_type = FROM_FILE;
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


