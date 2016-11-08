#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "pagedir.h"
#include "userprog/process.h"


static void syscall_handler (struct intr_frame *);
static void halt(void);
static bool remove(const char*);
static int open(const char*);
static int file_descriptor_number = 1;
static void close(int fd);
struct lock system_global_lock;
struct list files_opened;


void is_valid(void* addr);
void exit(int status_code);
int write(int fd, const void *buffer, unsigned size);	
void is_valid_buff(void* buff, int size);
int seek(int fd, unsigned position);
int tell(int fd);


int write(int fd, const void *buffer, unsigned size){
	lock_acquire(&system_global_lock);
	if (fd == STDOUT_FILENO) {
    putbuf(buffer, size);
    lock_release(&system_global_lock);
    return size;
  } else if(fd == STDIN_FILENO){
  	lock_release(&system_global_lock);
  	return -1;
  } else {
  	//struct file* open_file = //find open file by process
  	//if (open_file == NULL){
  		lock_release(&system_global_lock);
  		return -1;
  	//}
  	//int res = file_write(open_file, buffer, size);
  	lock_release(&system_global_lock);
  	//return res;
  }
}

void is_valid(void* addr){
	if(addr == NULL){
		exit(-1);
	}
	if(!is_user_vaddr(addr)){
		exit(-1);
	} else if (pagedir_get_page(thread_current()->pagedir, addr) == NULL){
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

exit(int status_code){
	printf("%s: exit(%d)\n", thread_current()->name, status_code);
	thread_exit();
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	is_valid(f->esp);
	is_valid_buff(f->esp, sizeof(int));
	int syscall_num = *(int*)f->esp;
	void* next;
	switch(syscall_num){
		case SYS_HALT:
			{
				halt();
				break;
			}
		case SYS_EXIT:
			{
				next = (int*)f->esp+1;

				is_valid(next);
				int arg = *(int*)next;
				exit(arg);
				break;
			}
		case SYS_EXEC:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				char *cmd_line = *(char*)next;
				f->eax = process_execute(cmd_line);
				break;
			}
		case SYS_WAIT:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int pid = *(int*)next;
				break;
			}
		case SYS_CREATE:
			{
				is_valid((int*)f->esp + 1);
				is_valid((int*)f->esp + 2);
				if(*(int*)((int*)f->esp+1) == NULL){
					exit(-1);
				}
				char* file = (char *) *((int*)f->esp + 1);
				int initial_size = *((int*)f->esp + 2);
				bool res;
				lock_acquire(&system_global_lock);
				res = filesys_create(file,initial_size);
				lock_release(&system_global_lock);
				f->eax = res;
				break;
			}
		case SYS_REMOVE:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				char *file = *(char*)next;
				f->eax = remove((char*) *((int*)f->esp+1));
				break;
			}
		case SYS_OPEN:
			{
				//printf("%s\n", "shshsh");
				//uint32_t * esp = f->esp;
				char * file = ((char *) *((int**)f->esp + 1));
				f->eax = open(file);
				//printf("%s\n", "jjjj");
				break;
			}
		case SYS_FILESIZE:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				break;
			}
		case SYS_READ:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				next = next+1;
				next = next+1;
				is_valid(next);
				int size = *(int*)((int*)f->esp+3);
				void* buf = *(char**)((int*)f->esp+2);
				is_valid_buff(buf, size);
				break;
			}
		case SYS_WRITE:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				next = next+1;
				next = next+1;
				is_valid(next);
				int size = *(int*)((int*)f->esp+3);
				void* buf = *(char**)((int*)f->esp+2);
				is_valid_buff(buf, size);
				f->eax = write(fd, buf, size);
				break;
			}
		case SYS_SEEK:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				next = (int*)f->esp+1;
				is_valid(next);
				unsigned position = *(unsigned*)next;
				seek(fd, position);
				break;
			}
		case SYS_TELL:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				f->eax = tell(fd);
				break;
			}
		case SYS_CLOSE:
			{
				next = (int*)f->esp+1;
				
				int fd = *(int*)next;
				//close(fd);
				break;
			}
		default:
			exit(-1);
	}
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

int open(const char* name){
	is_valid(name);
	lock_acquire(&system_global_lock);
	struct file * my_file = filesys_open(name); 
	if(my_file != NULL){
		struct thread* curThread = thread_current();
		struct file_descriptor * my_fd = malloc(sizeof(struct file_descriptor));
		my_fd->f = my_file;
		my_fd->id = curThread->fd_num;
		curThread->fd_num++;
		list_push_back(&files_opened,&my_fd->elem);
		list_push_back(&curThread->fd_list,&my_fd->elem);
		lock_release(&system_global_lock);
		return my_fd->id;
	}
	lock_release(&system_global_lock);
	return -1;
}


struct file_descriptor* find_my_descriptor(int fd){
	struct file_descriptor* res;
	//get descriptor owned by this process
}


int tell(int fd){
	if(fd < 0){
		exit(-1);
		return -1;
	}
	lock_acquire(&system_global_lock);
	struct file_descriptor* open_desc = find_my_descriptor(fd);
	if(open_desc != NULL){
		struct file* open_file = open_desc->f;
		int res = file_tell(open_file);
		lock_release(&system_global_lock);
		return res;
	}
	lock_release(&system_global_lock);
	return -1;
}

int seek(int fd, unsigned position){
	if(fd < 0){
		exit(-1);
	}
	lock_acquire(&system_global_lock);
	struct file_descriptor* open_desc = find_my_descriptor(fd);
	if(open_desc != NULL){
		struct file* open_file = open_desc->f;
		file_seek(open_file, position);
	}
	lock_release(&system_global_lock);
}







