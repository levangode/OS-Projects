#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "pagedir.h"


static void syscall_handler (struct intr_frame *);
static void halt(void);
static bool remove(const char*);
static int open(const char*);
static int file_descriptor_number = 1;
struct lock system_global_lock;
struct list files_opened;


void is_valid(void* addr);
static void exit(int status_code);
static int write(int fd, const void *buffer, unsigned size);	
void is_valid_buff(void* buff, int size);


static int write(int fd, const void *buffer, unsigned size){
	lock_acquire (&system_global_lock); 
	if (fd == STDOUT_FILENO){
    putbuf((char*)buffer, (size_t)size);
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
  list_init(&files_opened);
}
	//todo return code to parent
void change_child_from_parent(int status_code, struct thread* cur_thread,struct thread* parent_thread){
	struct list_elem* elem = list_head(&parent_thread->child_list);
	for(; elem != list_tail(&parent_thread->child_list); elem = list_next(elem)){
		struct child_info* child = list_entry(elem,struct child_info,elem_list_stat);
		if(child->child_id == cur_thread->tid){
			lock_acquire(&parent_thread->child_lock);
			child->exit_status = status_code;
			child->is_exit = true;
			lock_release(&parent_thread->child_lock);
			return;
		}
	}
}


void exit(int status_code){
	struct thread *cur_thread = thread_current();
	struct thread *parent_thread = thread_get(cur_thread->parent_id);		
	if(parent_thread!=NULL){
		change_child_from_parent(status_code,cur_thread,parent_thread);	
	}
	printf("%s: exit(%d)\n", thread_current()->name, status_code);
	thread_exit();
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{

	is_valid(f->esp);
	int syscall_num = *(int*)f->esp;
	switch(syscall_num){
		case SYS_HALT:
			halt();
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
			f->eax = remove((char*) *((int*)f->esp+1));
			break;
		case SYS_OPEN:
			f->eax = open( (char*)*((int*)f->esp+1));
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

			void* buf = *(char**)((int*)f->esp+2);
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

static void halt(void){
	shutdown_power_off();
}

static bool remove(const char* name){
	is_valid(name);
	bool res;
	lock_acquire(&system_global_lock);
	res = filesys_remove(name);
	lock_release(&system_global_lock);
	return res;	
}

static int open(const char* name){
	is_valid(name);
	lock_acquire(&system_global_lock);
	struct file * my_file = filesys_open(name); 
	if(my_file != NULL){
		struct fd * file_descriptor = calloc(1,sizeof(*file_descriptor));
		file_descriptor->id = file_descriptor_number;
		file_descriptor_number++;
		file_descriptor->master = thread_current()->tid;
		file_descriptor->f = my_file;
		list_push_back(&files_opened,&file_descriptor->elem);
		lock_release(&system_global_lock);
		return file_descriptor->id;
	}
	lock_release(&system_global_lock);
	return -1;
}




