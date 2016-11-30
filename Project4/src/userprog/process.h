#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

#ifdef VM
	struct map_descriptor {
	  int descriptor_id;
	  struct file* file;

	  void *map_pointer;   // pointer to where memmory is mapped.
	  size_t file_size;  // file size

	  struct list_elem elem;
	};

#endif

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

void push_to_stack(char** argv, int argc, void** esp);
void set_status_code(int status_code);
bool install_page (void *upage, void *kpage, bool writable);

#endif /* userprog/process.h */
