// See LICENSE for license details.

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "file.h"
#include "bbl.h"
#include "vm.h"
#include "frontend.h"
#include "syscall.h"
#include "mtrap.h"
// #include <string.h>

#define MAX_FILES 32
file_t files[MAX_FILES];
spinlock_t file_lock = SPINLOCK_INIT;
spinlock_t refcnt_lock = SPINLOCK_INIT;
FATFS Fs_Local [2];
static char croot[FF_MAX_LFN];
static char filenames [MAX_FILES][FF_MAX_LFN];

#define erase_leading_backslash(path) \
  if((path) && *(path)=='/') \
    (path)++ \


int file_chdir(const char* path)
{
  //erase_leading_backslash(path);
  FRESULT fr = (FRESULT)frontend_syscall(SYS_chdir,va2pa(path),0,0,0,0,0,0);
  if(fr != FR_OK){
    printk("chdir failed at path %s, errcode %ld.\n",path,fr);
    return -ENOENT;
  }
  strcpy(croot,path);
  return 0;
}

char * file_getcwd(char* buf, size_t len)
{
  FRESULT fr = (FRESULT)frontend_syscall(SYS_getcwd,va2pa(buf),len,0,0,0,0,0);
  if(fr != FR_OK){
    printk("getcwd failed. Errcode %ld.\n",fr);
    return NULL;
  }
  strcpy(croot,buf);
  return buf;
}

int file_fcntl(int fd, int cmd, uint64_t a0)
{
  int fr = -1;
  file_t * f = file_get(fd);
  
  if(f) {
    fr = (int) frontend_syscall(SYS_fcntl,va2pa(&f->fd),cmd,a0,0,0,0,0);
    file_decref(f);
  }

  return fr;
}

int file_unlinkat(int dirfd, const char* pathname, int flags)
{
  //erase_leading_backslash(pathname);
  FRESULT fr = (FRESULT)frontend_syscall(SYS_unlink,va2pa(pathname),0,0,0,0,0,0);
  if (fr != FR_OK){
    printk("unlink failed. path is %s, errcode %ld.\n",pathname,fr);
    return -EINVAL;
  }
  return 0;
}

int file_reopen(int fd, const char* fn,int flags) 
{
  file_t * f;
  if (fd < 3) {
    f = &files[fd];
    if (atomic_cas(&f->refcnt, 0, 2) != 0){
      fd_close(fd);
      if (atomic_cas(&f->refcnt, 0, 2) != 0){
        f = NULL;
      }
    }
    
  }
  else 
    f = file_get(fd);

  //erase_leading_backslash(fn);

  if (f ){
    uint8_t mode = 0;
    if((flags & O_ACCMODE) == O_RDONLY) mode = FA_READ;
    if((flags & O_ACCMODE) == O_WRONLY) mode = FA_WRITE;
    if((flags & O_ACCMODE) == O_RDWR)   mode = FA_READ | FA_WRITE ;
    if(flags & O_EXCL) mode |= FA_CREATE_NEW;
    else if (flags & O_TRUNC) mode |= FA_CREATE_ALWAYS;
    else if (flags & O_CREAT) mode |= FA_OPEN_ALWAYS;

    if (fd == 0){
      mode = FA_READ;
    }
    else if (fd == 1 || fd == 2){
      mode = FA_WRITE|FA_OPEN_ALWAYS;
    }

    FRESULT rt = (FRESULT) frontend_syscall(SYS_open,va2pa(&f->fd),va2pa(fn),mode,0,0,0,0);
    if (rt) {
      printk("fail to reopen file %s at fd %d,errcode %d\n", fn,fd, rt);
      file_decref(f);
      return -1;
    } else {
      strncpy(filenames[fd],fn,FF_MAX_LFN);
      return 0;
    }

  }
  printk("No corresponding file of fd %d\n",fd);  
  return -1;
}


long file_mount()
{
  FRESULT fr = (FRESULT) frontend_syscall(SYS_mount,va2pa(&Fs_Local[0]),va2pa("0:"),1,0,0,0,0);
  // if(fr != FR_OK) printk("fail to mount SD file system, Err code %d.\n",fr);
  // else printk("SD file system successfully mounted.\n");
  return (long) fr;
}

long file_umount()
{
  FRESULT fr = (FRESULT) frontend_syscall(SYS_mount,0,va2pa("0:"),0,0,0,0,0);
  // if(fr != FR_OK) printk("fail to mount SD file system, Err code %d.\n",fr);
  // else printk("SD file system successfully mounted.\n");
  return (long) fr;
}

void file_incref(file_t* f)
{
  long flags = spinlock_lock_irqsave(&refcnt_lock);
  long prev = f->refcnt;
  f->refcnt = prev + 1;
  spinlock_unlock_irqrestore(&refcnt_lock,flags);
  kassert(prev > 0);
}

void file_decref(file_t* f)
{
  long flags = spinlock_lock_irqsave(&refcnt_lock);
  long prev = f->refcnt;
  f->refcnt = prev - 1;
  if (prev == 2)
  {
    f->refcnt = 0;
    f->offset = 0;
    // f_close(&f->fd);
    memset(filenames[(f - files)],0,FF_MAX_LFN);
    frontend_syscall(SYS_close,va2pa(&f->fd),0,0,0,0,0,0);
  }
  spinlock_unlock_irqrestore(&refcnt_lock,flags);
}

static file_t* file_get_free()
{
  for (file_t* f = files+3; f < files + MAX_FILES; f++)
    if (atomic_cas(&f->refcnt, 0, 2) == 0)
      return f;
  panic("fail to get a free file");
  return NULL;
}

file_t* file_get(int fd)
{
  file_t* f = &files[fd];
 
  if (fd >= MAX_FILES )
    return 0;

  long old_cnt;
  do {
    old_cnt = atomic_read(&f->refcnt);
    if (old_cnt == 0)
      return 0;
  } while (atomic_cas(&f->refcnt, old_cnt, old_cnt+1) != old_cnt);

  return f;
}

int fd_close(int fd)
{
  file_t* f = file_get(fd);
  if (!f)
    return -1;
  file_decref(f);
  file_decref(f);
  return 0;
}

void inline file_display () {
  // int i; 
  // for (i = 0; i< MAX_FILES; i++){
  //   printk("files[%d].offset=%ld;refcnt=%ld;name=%s\n",i,files[i].offset,files[i].refcnt,filenames[i]);
  // }
}

void file_init()
{
  int i;
  for(i=0; i<MAX_FILES; i++) {
    files[i].offset = 0;
    files[i].refcnt = 0;
  }

  // static FATFS fs_loc;

  // FRESULT fr;
  // fr = f_mount(&fs_loc,"0:",1);
  // // if(fr != FR_OK) printk("fail to mount SD driver. err code %d.\n",fr);
  
  // fr = f_mount(&fs_loc,"0:",1);
  // // if(fr != FR_OK) printk("fail to mount SD driver a second time. err code %d.\n",fr);
  // // else printk("mount succeed at the second time.\n");
}

file_t* file_openat(int dirfd, const char* fn, int flags)
{
  file_t* f = file_get_free();
  if (f == NULL)
    return f;

  //erase_leading_backslash(fn);
  
  uint8_t mode = 0;
  if((flags & O_ACCMODE) == O_RDONLY) mode = FA_READ;
  if((flags & O_ACCMODE) == O_WRONLY) mode = FA_WRITE;
  if((flags & O_ACCMODE) == O_RDWR)   mode = FA_READ | FA_WRITE ;
  if(flags & O_EXCL) mode |= FA_CREATE_NEW;
  else if (flags & O_TRUNC) mode |= FA_CREATE_ALWAYS;
  else if (flags & O_CREAT) mode |= FA_OPEN_ALWAYS;

  // FRESULT rt = f_open(&f->fd, fn, mode); /* check mode */
  FRESULT rt = (FRESULT) frontend_syscall(SYS_open,va2pa(&f->fd),va2pa(fn),mode,0,0,0,0);
  if (rt) {
    printk("fail to open file %s,errcode %d\n", fn,rt);
    file_decref(f);
    return NULL;
  } else {
    strncpy(filenames[(f-files)],fn,FF_MAX_LFN);
    return f;
  }
}

file_t* file_open(const char* fn, int flags)
{
  return file_openat(AT_FDCWD, fn, flags);
}

ssize_t file_read(file_t* f, void* buf, size_t size)
{
  // printk("buf vaddr %p, paddr %p\n",buf, va2pa(buf));
  // printk("file read offset is %ld\n",f->offset);
  // printk("file read offset in low-level implementation is %ld\n",f_tell(&f->fd));
  populate_mapping(buf, size, PROT_WRITE);
  long flags = spinlock_lock_irqsave(&file_lock);
  // f_lseek(&f->fd, f->offset);
  frontend_syscall(SYS_lseek,va2pa(&f->fd),f->offset,0,0,0,0,0);
  uint32_t rsize;
  // f_read(&f->fd, buf, size, &rsize);
  FRESULT fr ;
  fr = (FRESULT) frontend_syscall(SYS_read,va2pa(&f->fd),va2pa(buf),size,va2pa(&rsize),0,0,0);
  f->offset += rsize;
  spinlock_unlock_irqrestore(&file_lock,flags);
  // printk("f_read returns %ld\n",fr);
  // printk("rsize @ vaddr %p, paddr %p\n",&rsize,va2pa(&rsize));
  // printk("bytes read : %ld\n",rsize);
  return rsize;
}

ssize_t file_pread(file_t* f, void* buf, size_t size, off_t offset)
{
  populate_mapping(buf, size, PROT_WRITE);
  long flags = spinlock_lock_irqsave(&file_lock);
  // FRESULT rt = f_lseek(&f->fd, offset);
  FRESULT rt = (FRESULT) frontend_syscall(SYS_lseek,va2pa(&f->fd),offset,0,0,0,0,0);
  if(rt) panic("f_lseek() failed! %d", rt);
  uint32_t rsize = 0;
  // f_read(&f->fd, buf, size, &rsize);
  rt = (FRESULT) frontend_syscall(SYS_read,va2pa(&f->fd),va2pa(buf),size,va2pa(&rsize),0,0,0);
  if(rt) panic("f_read() failed! %d", rt);
  spinlock_unlock_irqrestore(&file_lock,flags);
  return rsize;
}

ssize_t file_write(file_t* f, const void* buf, size_t size)
{
  
  populate_mapping(buf, size, PROT_READ);
  long flags = spinlock_lock_irqsave(&file_lock);
  // f_lseek(&f->fd, f->offset);
  frontend_syscall(SYS_lseek,va2pa(&f->fd),f->offset,0,0,0,0,0);
  uint32_t wsize = 0;
  // f_write(&f->fd, buf, size, &wsize);
  frontend_syscall(SYS_write,va2pa(&f->fd),va2pa(buf),size,va2pa(&wsize),0,0,0);
  f->offset += wsize;
  spinlock_unlock_irqrestore(&file_lock,flags);
  return wsize;
}

ssize_t file_pwrite(file_t* f, const void* buf, size_t size, off_t offset)
{
  populate_mapping(buf, size, PROT_READ);
  long flags = spinlock_lock_irqsave(&file_lock);
  // f_lseek(&f->fd, offset);
  frontend_syscall(SYS_lseek,va2pa(&f->fd),offset,0,0,0,0,0);
  uint32_t wsize = 0;
  // f_write(&f->fd, buf, size, &wsize);
  frontend_syscall(SYS_write,va2pa(&f->fd),va2pa(buf),size,va2pa(&wsize),0,0,0);
  spinlock_unlock_irqrestore(&file_lock,flags);
  return wsize;
}

void stat_display(struct stat * s){
  printk("struct s @%p\n",s);
  printk("%s: %#x\n","st_mode",s->st_mode);
  printk("%s: %d\n","st_ino",s->st_ino);
  printk("%s: %#x\n","st_dev",s->st_dev);
  printk("%s: %d\n","st_nlink",s->st_nlink);
  printk("%s: %d\n","st_uid",s->st_uid);
  printk("%s: %d\n","st_gid",s->st_gid);
  printk("%s: %p\n","st_size",s->st_size);
  printk("%s: %ld\n","st_blocks",s->st_blocks);
  printk("%s: %ld\n","st_blksize",s->st_blksize);
  printk("%s: %p\n","st_atime",s->st_atime);
  printk("%s: %p\n","st_mtime",s->st_mtime);
  printk("%s: %p\n","st_ctime",s->st_ctime);
}

int file_stat(file_t * f, struct stat* s)
{
 populate_mapping(s, sizeof(*s), PROT_WRITE);
//  FRESULT rt = f_stat(fn, s);   /* check struct of stat */
 FRESULT rt = (FRESULT) frontend_syscall(SYS_stat,va2pa(&Fs_Local[0]),va2pa(filenames[(f-files)]),va2pa(s),0,0,0,0);
//  printk("fstat: f_stat returns %ld\n",rt);
 switch(rt){
  case FR_NO_PATH:
  case FR_NO_FILE:
    return -ENOENT;
  case FR_INVALID_NAME:
    return -ENOTDIR;
  case FR_DISK_ERR:
    return -EFAULT;
  case FR_OK:
    break;
  default:
    return -ENOENT;
 }

 s->st_mode |= S_IRWXG | S_IRWXO | S_IRWXU ;
 s->st_ino = (f-files); // not supported;
 s->st_nlink = 1; // not supported;
 s->st_uid = 0;
 s->st_gid = 0;

//  stat_display(s);

 return 0;

}

int file_statat(int dirfd, const char* path,struct stat * s)
{
  populate_mapping(s, sizeof(*s), PROT_WRITE);
  //erase_leading_backslash(path);
 FRESULT rt = (FRESULT) frontend_syscall(SYS_stat,va2pa(&Fs_Local[0]),va2pa(path),va2pa(s),0,0,0,0);
//  printk("fstatat: f_stat returns %ld\n",rt);
 switch(rt){
  case FR_NO_PATH:
  case FR_NO_FILE:
    return -ENOENT;
  case FR_INVALID_NAME:
    return -ENOTDIR;
  case FR_DISK_ERR:
    return -EFAULT;
  case FR_OK:
    break;
  default:
    return -EINVAL;
 }

 s->st_mode |= S_IRWXG | S_IRWXO | S_IRWXU ;
 s->st_ino = 0; // not supported;
 s->st_nlink = 1; // not supported;
 s->st_uid = 0;
 s->st_gid = 0;

//  stat_display(s);

 return 0;
}

int file_accessat(int dirfd, const char* name, int mode)
{
  static struct stat s;
  populate_mapping(&s, sizeof(s),PROT_WRITE);
  //erase_leading_backslash(name);
  //Abuse: use frontend syscall SYS_stat to check whether the file exists.
  // If it exists, then all accesses to it are permitted.
  FRESULT rt = (FRESULT) frontend_syscall(SYS_stat,va2pa(&Fs_Local[0]),va2pa(name),va2pa(&s),0,0,0,0);
  // printk("faccessat: f_stat returns %ld\n",rt);
  switch(rt){
    case FR_NO_PATH:
    case FR_NO_FILE:
      return -ENOENT;
    case FR_INVALID_NAME:
      return -ENOTDIR;
    case FR_DISK_ERR:
      return -EFAULT;
    case FR_OK:
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

int file_truncate(file_t* f, off_t len)
{
  panic("file_truncate() not supported!");
  return -1;
}

ssize_t file_lseek(file_t* f, size_t ptr, int dir)
{
  FRESULT rt;
  if(dir == SEEK_SET) {
    // rt = f_lseek(&f->fd, ptr);
    rt = (FRESULT) frontend_syscall(SYS_lseek,va2pa(&f->fd),ptr,0,0,0,0,0);
    if(rt) return -1;
    else return (f->offset = ptr);
  } else if(dir == SEEK_CUR) {
    // rt = f_lseek(&f->fd, f->offset + ptr);
    rt = (FRESULT) frontend_syscall(SYS_lseek,va2pa(&f->fd),f->offset + ptr,0,0,0,0,0);
    if(rt) return -1;
    else return (f->offset += ptr);
  } else {
    // panic("lseek SEEK_END not supported!");
    // return -1;
    // rt = f_lseek(&f->fd, f_size(&f->fd) + ptr);
    rt = (FRESULT) frontend_syscall(SYS_lseek,va2pa(&f->fd),f_size(&f->fd)+ptr,0,0,0,0,0);  
    if(rt) return -1;
    else return (f->offset = f_size(&f->fd) + ptr);
  }
}
