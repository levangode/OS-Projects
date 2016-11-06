#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "pagedir.h"


static void syscall_handler (struct intr_frame *);

struct lock system_global_lock;



void is_valid(void* addr);
static void exit(int status_code);
static int write(int fd, const void *buffer, unsigned size);	
void is_valid_buff(void* buff, int size);


static int write(int fd, const void *buffer, unsigned size){
	lock_acquire (&system_global_lock); 
	if (fd == STDOUT_FILENO){
    putbuf(buffer, size);
  }
  lock_release (&system_global_lock);
  return size;
}

void is_valid(void* addr){
	if(!is_user_vaddr(addr)){
		exit(-1);
	}
}

void is_valid_buff(void* buff, int size){
  char* iterator = (char*) buff;
  int i;
  for (i = 0; i<size; i++){
  	is_valid(iterator+i);
  }
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&system_global_lock);
}


static void exit(int status_code){
	//todo return code to parent
	printf("%s: exit(%d)\n", thread_current()->name, status_code);
	thread_exit();
}



int user_to_kernel_ptr(const void *vaddr)
{
  // TO DO: Need to check if all bytes within range are correct
  // for strings + buffers
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      exit(-1);
    }
  return (int) ptr;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{

	is_valid(f->esp);
	int syscall_num = *(int*)f->esp;
	switch(syscall_num){
		case SYS_HALT:
			break;
		case SYS_EXIT:
			;
			void* next = (int*)f->esp+1;

			is_valid(next);
			int arg = *(int*)next;
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
			;
			next = (int*)f->esp+1;
			is_valid(next);
			int fd = *(int*)next;
			next = next+1;
			next = next+1;
			is_valid(next);
			int size = *(int*)(f->esp+3);

			char* buf = *(char**)((int*)f->esp+2);
			is_valid_buff(buf, size);
			f->eax = write(fd, buf, size);
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
