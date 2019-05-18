#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <devices/shutdown.h>
#include <threads/vaddr.h>
#include <filesys/filesys.h>
#include <string.h>
#include <filesys/file.h>
#include <devices/input.h>
#include <threads/palloc.h>
#include <threads/malloc.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "pagedir.h"

static void syscall_handler (struct intr_frame *);
static void check_address(void *p);
struct opened_file * get_file(int fd);

static void sys_halt(void);
static void sys_exit(int *p);
static uint32_t sys_exec(int *p);
static uint32_t sys_wait(int *p);
static uint32_t sys_create(int *p);
static uint32_t sys_remove(int *p);
static uint32_t sys_open(int *p);
static uint32_t sys_filesize(int *p);
static uint32_t sys_read(int *p);
static uint32_t sys_write(int *p);
static void sys_seek(int *p);
static uint32_t sys_tell(int *p);
static void sys_close(int *p);
static void unexpected_exit(void);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void unexpected_exit(){
  thread_current()->exit_status = -1;
  thread_exit();
}


void check_address(void *p){
  if(p == NULL || !is_user_vaddr(p)) unexpected_exit();
}

struct opened_file * get_file(int fd){
  struct list_elem *e;
  struct opened_file *fn = NULL;
  struct list *files = &thread_current()->opened_files;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
    fn = list_entry (e, struct opened_file, file_elem);
    if (fd == fn->fd) return fn;
  }
  return NULL;
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int *p = f->esp;
  int syscall_type = *p++;

  switch (syscall_type){
    case SYS_HALT:
        sys_halt();
      break;

    case SYS_EXIT:
        sys_exit(p);
      break;

    case SYS_EXEC:
        f->eax = sys_exec(p);
      break;

    case SYS_WAIT:
        f->eax = sys_wait(p);
      break;

    case SYS_CREATE:
        f->eax = sys_create(p);
      break;

    case SYS_REMOVE:
        f->eax = sys_remove(p);
      break;

    case SYS_OPEN:
        f->eax = sys_open(p);
      break;

    case SYS_FILESIZE:
        f->eax = sys_filesize(p);
      break;

    case SYS_READ:
        f->eax = sys_read(p);
        break;

    case SYS_WRITE:
        f->eax = sys_write(p);
      break;

    case SYS_SEEK:
        sys_seek(p);
      break;

    case SYS_TELL:
        f->eax = sys_tell(p);
      break;

    case SYS_CLOSE:
        sys_close(p);
      break;

    default:
      unexpected_exit();
  }
}

static void sys_halt(){
    shutdown_power_off();
}

static void sys_exit(int *p){
    if (!is_user_vaddr(p)) unexpected_exit();
    thread_current()->exit_status = *p;
    thread_exit();
}

static uint32_t sys_exec(int *p){
    return process_execute((char*)*p);
}

static uint32_t sys_wait(int *p){
    if (!is_user_vaddr(p)) unexpected_exit();
    return process_wait(*p);
}

static uint32_t sys_create(int *p){
    check_address((void *) *p);
    return filesys_create((const char *)*(p+3),*(p+4));
}

static uint32_t sys_remove(int *p){
    return filesys_remove((const char *)*p);
}

static uint32_t sys_open(int *p){
    check_address((void *) *p);
    struct thread *cur = thread_current();
    acquire_file_lock();
    struct file * f = filesys_open((const char *)*p);
    release_file_lock();
    if(f != NULL) {
        struct opened_file *fn = malloc(sizeof(struct opened_file));
        fn->fd = cur->usable_fd;
        cur->usable_fd = cur->usable_fd + 1;
        fn->file = f;
        list_push_back(&cur->opened_files, &fn->file_elem);
        return fn->fd;
    }
    return -1;
}

static uint32_t sys_filesize(int *p){
    struct opened_file * f = get_file(*p);
    if (f != NULL) return file_length(f->file);
    return -1;
}

static uint32_t sys_read(int *p){
    check_address((void *) *(p + 1));
    int fd = *p;
    uint8_t * buffer = (uint8_t*)*(p + 1);
    off_t size = *(p + 2);

    uint32_t answer;
    if (fd != STDIN_FILENO) {
        struct opened_file * f = get_file(*p);
        if (f != NULL) answer = file_read(f->file, buffer, size);
        else answer = -1;
    }
    else{
        for (int i=0; i<size; i++) buffer[i] = input_getc();
        answer = size;
    }
    return answer;
}

static uint32_t sys_write(int *p){
    int fd = *p;
    const char * buffer = (const char *)*(p+5);
    off_t size = *(p+2);

    uint32_t answer;
    if (fd == STDOUT_FILENO) {
        putbuf(buffer,size);
        answer = 0;
    }
    else{
        struct opened_file *f = get_file(*p);
        if (f != NULL){
            answer = file_write(f->file, buffer, size);
        } else
            answer = 0;
    }
    return answer;
}

static void sys_seek(int *p){
    struct opened_file *f = get_file(*p);
    if (f != NULL) file_seek(f->file, *(p+1));
}

static uint32_t sys_tell(int *p){
    struct opened_file *f = get_file(*p);
    if (f != NULL) return file_tell(f->file);
    return -1;
}

static void sys_close(int *p){
    struct opened_file *f = get_file(*p);
    if (f != NULL){
        file_close(f->file);
        list_remove(&f->file_elem);
    }
}