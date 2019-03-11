#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"




//#include "lib/kernel/console.c"




 void syscall_handler (struct intr_frame *);
void
syscall_init (void) 
{
  lock_init(&fsLock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
 bool create (const char *file, unsigned initial_size){
  lock_acquire(&fsLock);
  bool ret= filesys_create(file,initial_size);;
  lock_release(&fsLock);
  return ret;
}
 bool removeF(const char *file) {
 lock_acquire(&fsLock);
  bool ret =filesys_remove(file);
   lock_release(&fsLock);
   return ret;
}
 int filesize (int fd){
  struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd)
   {
     lock_acquire(&fsLock);
     int fileLength =file_length(f->f);
     lock_release(&fsLock); 
    return fileLength;
   }
  }
  return -1;
}

 void close (int fd) {
  struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd)
    {
      lock_acquire(&fsLock);
    file_close(f->f);
    lock_release(&fsLock);
    }
    list_remove(elemf);
  }
}
 int open (const char *file) {
  struct file* currentFile =filesys_open(file);
  lock_acquire(&fsLock);
  if(currentFile!=NULL){
  struct file_p* fp=malloc(sizeof(struct file_p));
  fp->f=currentFile;
  int fileDVal =thread_current()->fd; 
  fp->fd=fileDVal;
  thread_current()->fd++;
  list_push_back(&thread_current()->files_l,&fp->elem);
  lock_release(&fsLock);
  //free(fp);
  return fileDVal;
  }
  lock_release(&fsLock);
  return -1;
}
 int wait (pid_t pid){
  return process_wait(pid);
}
 void
syscall_handler (struct intr_frame *f UNUSED) 
{
  valid_addr_check(f->esp);
  int sys_call = * (int *) f->esp;
  int args[3];
  switch(sys_call){
    case SYS_HALT:
    shutdown_power_off();
    break;

    case SYS_EXIT:
    get_arguments(f,&args[0],1);
    exit(args[0]);
    break;

    case SYS_EXEC:
           get_arguments(f,&args[0],1);
        valid_addr_check((void*)args[0]);
  f->eax=exec((const char*)args[0]);
    break;

    case SYS_WAIT:
        get_arguments(f,&args[0],1);
    f->eax=wait(args[0]);
    break;

    case SYS_CREATE:
    get_arguments(f,&args[0],2);
    valid_addr_check((void*)args[0]);
    if(args[0]==NULL)
    exit(-1);
    f->eax=create((const char *)args[0],(unsigned)args[1]);
    break;

    case SYS_REMOVE:
     get_arguments(f,&args[0],1);
     valid_addr_check((void*)args[0]);
     f->eax =removeF((const char *)args[0]);
    break;

    case SYS_OPEN:
    get_arguments(f,&args[0],1);
    valid_addr_check((void*)args[0]);
    f->eax=open((const char *)args[0]);
    break;

    case SYS_FILESIZE:
         get_arguments(f,&args[0],1);
     // lock_acquire(&fsLock);
   f->eax=filesize (args[0]);
    //lock_release(&fsLock);
    break;

    case SYS_READ:
       get_arguments(f,&args[0],3);
       valid_addr_check((void*)args[1]);
  f->eax=read ((int)args[0], (void *)args[1], (unsigned )args[2]);
    break;

    case SYS_WRITE:
    get_arguments(f,&args[0],3);
       valid_addr_check((void*)args[1]);
    f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
    break;

    case SYS_SEEK:
    get_arguments(f,&args[0],2);
    seek((int)args[0],(unsigned)args[1]);
    break;

    case SYS_TELL:
    get_arguments(f,&args[0],1);
    f->eax=tell((int)args[0]);
    break;

    case SYS_CLOSE:
     get_arguments(f,&args[0],1);
     close(args[0]);
    break;
    
  }
  

  //printf ("system call!\n");
  //thread_exit ();
}
unsigned tell (int fd){
  struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd){
     lock_acquire(&fsLock);
      int pos=file_tell(f->f);
      lock_release(&fsLock);
      return pos;
   }
  }
  return -1;
}
void seek (int fd, unsigned position){
  struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd){
     lock_acquire(&fsLock);
      file_seek(f->f,position);
      lock_release(&fsLock);
      break;
   }
  }
}
int read (int fd, void *buffer, unsigned size){
  if(fd==0){
    uint8_t* bufferRead=(uint8_t *) buffer;
    for(int i=0;i<size;i++)
     bufferRead[i]= input_getc();
   return size;
  }
   struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd){
     lock_acquire(&fsLock);
      int s=file_read(f->f,buffer,size);
      lock_release(&fsLock);
      return s;
   }
  }
  return -1;
}
int write (int fd, const void *buffer, unsigned size) {
if(fd==1){
putbuf(buffer,size);
return size;
}else{
    struct file_p* f;
  for(struct list_elem *elemf=list_begin(&thread_current()->files_l);elemf!=list_end(&thread_current()->files_l);elemf=list_next(elemf)){
   f=list_entry(elemf,struct file_p,elem);
   if(f->fd==fd)
    {
      lock_acquire(&fsLock);
     int size_bytes=file_write(f->f,buffer,size);
     lock_release(&fsLock);
     return size_bytes;
    }
  }
}
return -1;
}

pid_t exec (const char *cmd_line) {
  char *fn=malloc(strlen(cmd_line)+1);
   strlcpy(fn,cmd_line,strlen(cmd_line)+1);
   char *saveptr;
   fn=strtok_r(fn," ",&saveptr);
   lock_acquire(&fsLock);
 struct file* f=filesys_open(fn);
 if(f!=NULL)
   file_close(f);
 lock_release(&fsLock);
 //free(fn);
 if(f==NULL)
 return -1;
 
return process_execute(cmd_line);
}
void exit(int status){
  struct thread *cur=thread_current()->pr;
 struct list_elem *childList=list_begin(&cur->child_l);
struct child_process *c;
 while(childList!=list_end(&cur->child_l)){
 c= list_entry(childList, struct child_process, elem);
  childList=list_next(childList);
 if(c->pid==thread_current()->tid){
  c->status_c=status;
   break;
 }
 }
  printf ("%s: exit(%d)\n",thread_current()->name,status);
  thread_exit();
}
void get_arguments(struct intr_frame *f,int* arg,int nums){
  for(int i=0;i<nums;i++){
    int* ar=(int*) f->esp+1+i;
    if(!is_user_vaddr(ar)) 
    {
     exit(-1);
     break;
  }
  else
      arg[i]=*ar;
  }
}
void valid_addr_check (void *add)
{
  if (!is_user_vaddr(add))
  { 
      exit(-1);
      return;
  }
  return user_kernel_addr_check(add);
}

void user_kernel_addr_check(void *add)
{
  void *page = pagedir_get_page(thread_current()->pagedir, add);
  if (page == NULL)
  {
      exit(-1);
  }
  return;
}

//proj2 ends
