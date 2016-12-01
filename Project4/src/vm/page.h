#ifndef _PAGE_
#define _PAGE_

#include <stdbool.h>
#include <hash.h>
#define FROM_FILE 0
#define ALL_ZERO 1
#define FROM_SWAP 2
#define FROM_MMAP 3

struct hash supplemental_page_table;



struct spt_entry{
	uint8_t* kpage;
	uint8_t* upage;
	int page_type;
	bool writable;
	bool isDirty;
	unsigned int bytes_read;
	unsigned int bytes_zero;
	struct file* f;
	size_t offset;
	size_t swap;
	struct hash_elem elem;
};

bool spt_install_file(void* upage,struct file* f,size_t offset,unsigned int bytes_read,unsigned int bytes_zero,bool writable);
struct spt_entry* find_page_in_supt(void * addr);
void page_init(struct hash* supplemental_page_table);
bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);
unsigned page_hash_func(const struct hash_elem *e, void *aux);

void spt_add(uint8_t* upage, uint8_t* kpage, bool writable);


bool stack_growth(uint8_t* uvaddr);

bool load_page(uint8_t* upage);



#endif