#include "page.h"
#include <hash.h>
#include <debug.h>
#include "threads/malloc.h"




void page_init(void){
	hash_init(&supplemental_page_table, page_hash_func, page_less_func, NULL);
}

struct spt_entry* find_page_in_supt(void * addr){
	struct spt_entry temp;
	temp.upage = addr;
	struct hash_elem * res = hash_find(&supplemental_page_table,&temp.elem);
	if(res != NULL){
		struct hash_elem* val = hash_entry(res,struct hash_entry,elem);
		return val;
	}
	return NULL;
}


unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED){
	struct spt_entry* entry = hash_entry(e, struct spt_entry, elem);
  	return hash_int((int)entry->upage);
}

bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux UNUSED){

	struct spt_entry* as = hash_entry(a, struct supt_entry, elem);
  	struct spt_entry* bs = hash_entry(b, struct supt_entry, elem);
	return as->upage < bs->upage;
}

void spt_add(uint8_t* upage, uint8_t* kpage, bool writable){
	struct spt_entry* tmp_entry = malloc(sizeof(struct spt_entry));
	
}