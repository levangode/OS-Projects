#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init (void);
struct file_descriptor{
	int id;
	struct file * f;
	tid_t master;
	struct list_elem elem;
};


#endif /* userprog/syscall.h */
