#ifndef _PAGE_
#define _PAGE_

#include <stdbool.h>
#include <hash.h>


struct hash supplemental_page_table;



struct supt_entry{
	uint8_t* kpage;
	uint8_t* upage;
	struct hash_elem elem;
};

void page_init(void);
bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);
unsigned page_hash_func(const struct hash_elem *e, void *aux);









#endif