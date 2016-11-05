#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
bool isValid(struct intr_frame* f);

bool isValid(struct intr_frame* f){
	if(is_user_vaddr(f->esp)){
		return true;
	}
	return false;
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	if(!isValid(f)){
		exit(-1);
	}   
	int syscall_num = *(int*)f->esp;
	switch(syscall_num){
		case SYS_HALT:
			break;
		case SYS_EXIT:
			int arg = *((int*)f->esp+1);
			exit(arg);
			break;
		case SYS_WAIT:
			break;
		case SYS_CREATE:
			break;
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			break;
		case SYS_WRITE:
			//int fd = *((int*)f->esp+1);
			//void* buffer = *(void*)(int*)f->esp+2;
			break;
		case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
	}
  printf ("system call!\n");
  thread_exit ();
}
