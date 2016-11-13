#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init (void);
struct file_descriptor{
	int id;
	struct file * f;
	struct list_elem elem;
};

void exit(int status_code);


#endif /* userprog/syscall.h */
