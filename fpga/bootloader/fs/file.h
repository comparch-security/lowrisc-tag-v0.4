// See LICENSE for license details.

#ifndef _FILE_H
#define _FILE_H

#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "atomic.h"

// FatFS
#include "ff.h"

typedef struct file
{
  FIL fd;                       /* FatFS file handler */
  uint32_t offset;              /* remember current fp position */
  unsigned refcnt;
} file_t;
extern file_t files[];
#define stdin  (files + 0)
#define stdout (files + 1)
#define stderr (files + 2)

#define AT_FDCWD -100

void file_incref(file_t* f);
void file_decref(file_t* f);
file_t* file_open(const char* fn, int flags);
file_t* file_openat(int dirfd, const char* fn, int flags);
ssize_t file_pwrite(file_t* f, const void* buf, size_t n, off_t off);
ssize_t file_pread(file_t* f, void* buf, size_t n, off_t off);
ssize_t file_write(file_t* f, const void* buf, size_t n);
ssize_t file_read(file_t* f, void* buf, size_t n);
ssize_t file_lseek(file_t* f, size_t ptr, int dir);
//int file_truncate(file_t* f, off_t len);
int file_stat(file_t* f, struct stat* s);
int file_statat(int dirfd, const char* path, struct stat* s);
int file_accessat(int dirfd, const char* name, int mode);
int file_chdir(const char* path);
char * file_getcwd(char * buf, size_t len);
int file_fcntl(int fd,int cmd, uint64_t a0);
int file_unlinkat(int dirfd, const char* pathname, int flags);
int file_reopen(int fd, const char* fn, int flags);


void file_init();
file_t* file_get(int fd);
int fd_close(int fd);

long file_mount();
long file_umount();
void file_display();

#endif
