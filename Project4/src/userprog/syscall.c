#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"


static void syscall_handler (struct intr_frame *);
void halt(void);
int open(const char* file_name);
void close(int fd);
void is_valid(void* addr);
void exit(int status_code);
int write(int fd, const void *buffer, unsigned size);	
void is_valid_buff(void* buff, int size);
void seek(int fd, unsigned position);
int tell(int fd);
int read(int fd, void* buffer, unsigned size);
bool remove(const char* file_name);
bool create(char* file, unsigned initial_size);
int open(const char* name);
void halt(void);
struct file_descriptor* find_my_descriptor(int fd);
int filesize(int fd);
struct file_descriptor * findFile(int file_descriptor_id, bool should_remove);





void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&system_global_lock);
}

/* Checks if the passed pointer is valid.
 * invalid if - null pointer, a pointer to unmapped virtual memory, or a pointer to kernel virtual
 * address space
 */
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

/* Also validates the pointer, iterating through the whole buffer */
void is_valid_buff(void* buff, int size){
  char* iterator = (char*) buff;
  int i;
  for (i = 0; i<size; i++){
  	is_valid(iterator+i);
  }
}


/* Exits from current thread. On exit prints his name and status code.
 * Frees(makes available) the file that is currently being used by this thread.
 */
void exit(int status_code){
	printf("%s: exit(%d)\n", thread_current()->name, status_code);
	set_status_code(status_code);
	if (thread_current()->current_file != NULL) {
    	file_allow_write(thread_current()->current_file);
  	}
	thread_exit();
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	is_valid(f->esp);
	is_valid_buff(f->esp, sizeof(int));
	int syscall_num = *(int*)f->esp;	//loads syscall number.
	thread_current()->backup_esp = f->esp;
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
				char *cmd_line = *(char**)next;
				
				f->eax = process_execute(cmd_line);
				
				break;
			}
		case SYS_WAIT:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int pid = *(int*)next;
				f->eax = process_wait(pid);
				break;
			}
		case SYS_CREATE:
			{
				is_valid((int*)f->esp + 1);
				is_valid((int*)f->esp + 2);
				char* file = *(char **)((int*)f->esp + 1);
				int initial_size = *((int*)f->esp + 2);
				f->eax = create(file, initial_size);
				break;
			}
		case SYS_REMOVE:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				char *file = *(char**)next;
				f->eax = remove(file);
				break;
			}
		case SYS_OPEN:
			{
				is_valid((int*)f->esp+1);
				char * file = *(char**)((int*)f->esp + 1);
				f->eax = open(file);
				break;
			}
		case SYS_FILESIZE:
			{
				next = (int*)f->esp+1;
				is_valid(next);
				int fd = *(int*)next;
				f->eax = filesize(fd);
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
				int size = *(unsigned int*)((int*)f->esp+3);
				void* buf = *(char**)((int*)f->esp+2);
				is_valid_buff(buf, size);
				f->eax=read(fd, buf, size);
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
				int size = *(unsigned int*)((int*)f->esp+3);
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
				next = (int*)f->esp+2;
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
				is_valid(next);
				int fd = *(int*)next;
				close(fd);
				break;
			}

#ifdef VM
		case SYS_MMAP: // 13
    		{
      			next = (int*)f->esp + 1;
    			int fd = *(int*)next;
    			next = (int*)f->esp + 2;
    			void* map_pointer = *(void**)next; 
      			
      			f->eax = mmap(fd, map_pointer);
      			break;
    		}

  		case SYS_MUNMAP: // 14
	    	{
	    		next = (int*)f->esp + 1;
    			int id = *(int*)next;

	    		f->eax = munmap(id);
	      		break;
	    	}
#endif
		default:
			exit(-1);
	}
}

#ifdef VM
int mmap(int fd, int address_to_map){
	// basic check of argument validity
	if(upage == NULL){
		return -1;
	} else 
	if(pg_ofs(upage) != 0) {
		return -1;
	} else 
	if(fd == 0 || fd == 1) {
		return -1;
	}

	struct thread* cur_t = thread_current();

	// interaction with file system requred.
	lock_acquire(&filesys_lock);



	//PANIC("trying to map file: %d on address: %u;", fd, address_to_map);
	lock_release(&filesys_lock);
    return 0;  
}

int munmap(int id){
	PANIC("trying to unmap map id: %d;", id);
	return 0;
}
#endif

struct file_descriptor * findFile(int file_descriptor_id, bool should_remove){
	struct thread* curThread = thread_current();
	struct list_elem * elem = list_begin(&curThread->fd_list);
	for(;elem != list_tail(&curThread->fd_list); elem = list_next(elem)){
		struct file_descriptor * fd = list_entry(elem,struct file_descriptor,elem);
		if(fd->id == file_descriptor_id){
			if(should_remove){
				list_remove(&fd->elem);
			}
			return fd;
		}
	}
	return NULL;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open
file descriptors, as if by calling this function for each one. */
void close(int file_descriptor_id){
	lock_acquire(&system_global_lock);	
	if(file_descriptor_id >=0){
		struct file_descriptor * fd = findFile(file_descriptor_id,true);
		if(fd != NULL && fd->f != NULL ){
			file_close(fd->f);
			lock_release(&system_global_lock);
			return;
		}
	}
	lock_release(&system_global_lock);
	exit(-1);
}

/* Creates a new file called file initially initial size bytes in size.*/
bool create(char* file, unsigned initial_size){
	bool res = false;
	if(file==NULL){
		exit(-1);
	}
	lock_acquire(&system_global_lock);
	res = filesys_create(file,initial_size);
	lock_release(&system_global_lock);
	return res;
}

/* Terminates Pintos by calling shutdown_power_off() */
void halt(void){
	shutdown_power_off();
}

/* Opens the file called file. Returns a nonnegative integer handle called a “file descriptor”
(fd), or -1 if the file could not be opened.*/
int open(const char* name){
	if(name == NULL){
		exit(-1);
	}
	lock_acquire(&system_global_lock);
	struct file * my_file = filesys_open(name); 
	if(my_file != NULL){
		struct thread* curThread = thread_current();
		struct file_descriptor * my_fd = malloc(sizeof(struct file_descriptor));
		my_fd->f = my_file;
		my_fd->id = curThread->fd_num;
		curThread->fd_num++;
		
		list_push_back(&curThread->fd_list,&my_fd->elem);
		lock_release(&system_global_lock);
		return my_fd->id;
	}
	lock_release(&system_global_lock);
	return -1;
}

/* Finds the file descriptor by (fd) in this thread's fd_list */
struct file_descriptor* find_my_descriptor(int fd){
	struct thread* cur_thread = thread_current();
	struct list* fd_list = &cur_thread->fd_list;
	struct list_elem* next = list_head(fd_list);
	while(next != list_tail(fd_list)){
		struct file_descriptor* desc = list_entry(next, struct file_descriptor, elem);
		if(desc->id == fd){
			return desc;
		} 
		next = list_next(next);
	}
	return NULL;
}
/* Returns the position of the next byte to be read or written in open file fd, expressed
in bytes from the beginning of the file. */
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

/* Changes the next byte to be read or written in open file fd to position, expressed in
bytes from the beginning of the file. */
void seek(int fd, unsigned position){
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

/* Returns the size, in bytes, of the file open as fd. */
int filesize(int fd){
	if(fd < 0){
		exit(-1);
	}
	lock_acquire(&system_global_lock);
	struct file_descriptor* open_desc = find_my_descriptor(fd);
	if(open_desc != NULL){
		struct file* open_file = open_desc->f;
		int res = file_length(open_file);
		lock_release(&system_global_lock);
		return res;
	}
	lock_release(&system_global_lock);
	return -1;
}

/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes
actually read. */
int read(int fd, void* buffer, unsigned size){
	if (fd == STDIN_FILENO){
    uint8_t* buff = (uint8_t *) buffer;
    int i;
    int s = size;
    for (i = 0; i < s; i++){
	  	buff[i] = input_getc();	//getchar returns uint8_t
		}
    return size;
  }
	lock_acquire(&system_global_lock);
	struct file_descriptor* desc = find_my_descriptor(fd);
	if(desc != NULL){
		struct file* file_of_fd = desc->f;
		int res = file_read(file_of_fd, buffer, size);
		lock_release(&system_global_lock);
		return res;
	}
	lock_release(&system_global_lock);
	return -1;	
}


/* Deletes the file called file. Returns true if successful, false otherwise. */
bool remove(const char* file_name){
	lock_acquire(&system_global_lock);
	if(file_name != NULL){
		bool res = filesys_remove(file_name);
		lock_release(&system_global_lock);
		return res;
	}
	lock_release(&system_global_lock);
	exit(-1);
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes actually
written, which may be less than size if some bytes could not be written. */
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
  	struct file_descriptor* open_desc = find_my_descriptor(fd);
		if(open_desc != NULL){
			struct file* open_file = open_desc->f;
			int res = file_write(open_file, buffer, size);
			lock_release(&system_global_lock);
			return res;
		} else {
			lock_release(&system_global_lock);
			return -1;
		}
	}
}





