#ifndef _SWAP_
#define _SWAP_
#include <stddef.h>
void swap_init(void);
size_t swap_out(void *swap_pg);
void swap_in(size_t index, void *swap_pg);
void swap_free(size_t index);

#endif