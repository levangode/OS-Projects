#ifndef _SWAP_
#define _SWAP_
void swap_init (void);
int swap_out (void *swap_pg);
void swap_in (int index, void *swap_pg);
void swap_free (int index);

#endif