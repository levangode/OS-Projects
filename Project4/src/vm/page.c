#include "page.h"
#include <hash.h>
#include <debug.h>




void page_init(void){
	hash_init(&supplemental_page_table, page_hash_func, page_less_func, NULL);
}


unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED){
	struct supt_entry* entry = hash_entry(e, struct supt_entry, elem);
  	return hash_int((int)entry->upage);
}

bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux UNUSED){

	struct supt_entry* as = hash_entry(a, struct supt_entry, elem);
  	struct supt_entry* bs = hash_entry(b, struct supt_entry, elem);
	return as->upage < bs->upage;
}