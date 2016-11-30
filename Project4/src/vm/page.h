#ifndef _PAGE_
#define _PAGE_

#include <stdbool.h>
#include <hash.h>


struct hash supplemental_page_table;



struct spt_entry{
	uint8_t* kpage;
	uint8_t* upage;

	bool writable;

	struct hash_elem elem;
};


struct spt_entry* find_page_in_supt(void * addr);
void page_init(void);
bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);
unsigned page_hash_func(const struct hash_elem *e, void *aux);

void spt_add(uint8_t* upage, uint8_t* kpage, bool writable);


bool stack_growth(uint8_t* uvaddr);



#endif