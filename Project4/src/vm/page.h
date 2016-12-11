#ifndef _PAGE_
#define _PAGE_

#include <stdbool.h>
#include <hash.h>
#include <inttypes.h>
#include "threads/palloc.h"
#include "threads/thread.h"




#define FROM_FILE 0
#define ALL_ZERO 1
#define FROM_SWAP 2
#define FROM_MMAP 3

#define STACK_LIMIT 0x800000  // == 2^23 == 8mb

struct spt_entry{
	uint8_t* kpage;
	uint8_t* upage;
	int page_type;
	bool writable;
	bool isDirty;
	size_t bytes_read;
	size_t bytes_zero;
	struct file* f;
	off_t offset;
	size_t swap;
	bool loaded;
	
	struct hash_elem elem;
	bool isSwap;
};

bool spt_install_file(void* upage,struct file* f,off_t offset,size_t bytes_read,size_t bytes_zero,bool writable);
bool spt_install_file_mmap(void* upage,struct file* f,off_t offset,size_t bytes_read,size_t bytes_zero,bool writable);
struct spt_entry* find_page_in_supt(void * addr);
void page_init(struct hash* supplemental_page_table);
bool page_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);
unsigned page_hash_func(const struct hash_elem *e, void *aux);

void spt_add(uint8_t* upage, uint8_t* kpage, bool writable);

void free_spt(struct hash* supplemental_page_table);

bool stack_growth(uint8_t* uvaddr);

bool load_page(uint8_t* upage);



#endif