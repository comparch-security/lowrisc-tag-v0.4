#include <stdint.h>
#include "ff.h"
#include "remove_htif.h"
#include "frontend.h"
#include "syscall.h"
#include "diskio.h"
#include <fcntl.h>


static uint64_t sys_fcntl(FIL* fd, int cmd, uint64_t a0)
{
  uint64_t ret_val;
  switch (cmd) {
    case F_DUPFD:
      printm("fcntl: file dup not supported!\n");
      ret_val = -1;
      break;
    case F_GETFD: 
      ret_val = 0; //FD_CLOEXEC is not defined.
      break;
    case F_SETFD:
      printm("fcntl: F_SETFD not supported!\n");
      ret_val = -1;
      break;
    case F_GETFL:
      switch((fd->flag)&(FA_READ|FA_WRITE)){
        case FA_READ|FA_WRITE:
          ret_val = O_RDWR;
          break;
        case FA_READ:
          ret_val = O_RDONLY;
          break;
        case FA_WRITE:
          ret_val = O_WRONLY;
          break;
        default:
          printm("fcntl: wrong fd flags.\n");
          ret_val = -1; 
          break;
      }
      break;
    case F_SETFL:
      printm("fcntl: F_SETFL not supported!\n");
      ret_val = -1;
      break;
    case F_GETOWN:
      ret_val = 0;
      break;
    case F_SETOWN:
      printm("fcntl: F_SETOWN not supported!\n");
      ret_val = -1;
      break;
    default:
      printm("fcntl: not supported cmd %x.\n",cmd);
      ret_val = -1;
      break;
  }
  return ret_val;
}
extern void* memset(void* dest, int byte, size_t len);
static uint64_t sys_stat(FATFS* fs,const TCHAR*path, struct stat * st)
{
  uint32_t blksize ;
  DRESULT dr = disk_ioctl(fs->drv,GET_BLOCK_SIZE,&blksize);
  if(dr != RES_OK){
    printm("disk_ioctl GET_BLOCK_SIZE failed. Errcode %ld.\n",dr);
    return FR_DISK_ERR;
  }
  
  static FILINFO fno ;
  memset(&fno,0,sizeof(fno));

  FRESULT fr = f_stat(path,&fno);
  if(fr != FR_OK ){
    printm("f_stat of path %s failed. Errcode %ld.\n",path,fr);
    return fr;
  }

  FILINFO * finfo = &fno;
  
  uint32_t blocks = (finfo->fsize + (blksize - 1) ) / blksize;
  
  uint32_t year,mon,day,hour,minute,second;
  year = (finfo->fdate >> 9) + 1980;
  mon = (finfo->fdate & 0x1e0) >> 5;
  day = finfo->fdate & 0x1f;
  hour = finfo->ftime >> 11;
  minute = (finfo->ftime & 0x7e) >> 5;
  second = (finfo->ftime & 0x1f) << 1;

  static unsigned accu_mon_day[12] = {
    0,31,59,90,120,151,181,212,243,273,304,334
  };

  //accurate between 1973-2099
  uint64_t time = ((year - 1973)/4 + 1) + (year - 1970)* 365 ;
  time += (year % 4 == 0 && mon > 2)?1:0 + accu_mon_day[mon-1];
  time += day - 1;
  time = time * 24 + hour;
  time = time * 60 + minute;
  time = time * 60 + second;

  st->st_dev = 0;
  st->st_rdev = 0;
  // take files with AM_DIR as directories, others as regular files.
  st->st_mode = (finfo->fattrib & AM_DIR)?(S_IFMT & S_IFDIR):(S_IFMT & S_IFREG); 
  st->st_size = finfo->fsize;
  st->st_atime = time; //No RTC
  st->st_ctime = time; //No RTC
  st->st_blksize = blksize;
  st->st_blocks = blocks;


  return 0;
}

void dispatch_htif_syscall(uintptr_t magic_mem){
  uint64_t * args = (uint64_t*) magic_mem;
  uint64_t no = args[0];
  switch (no){
    case SYS_read:
      // printm("&rsize in M-mode @ %p\n",(uint32_t*)args[4]);
      args[0] = (uint64_t)f_read((FIL*)args[1],(void*)args[2],(uint32_t)args[3],(uint32_t*)args[4]);
      break;
    case SYS_write: 
      args[0] = (uint64_t)f_write((FIL*)args[1],(const void*)args[2],(uint32_t)args[3],(uint32_t*)args[4]);
      break;
    case SYS_close: 
      args[0] = (uint64_t)f_close((FIL*)args[1]);
      break;
    case SYS_open:
      args[0] = (uint64_t)f_open((FIL*)args[1],(const TCHAR*)args[2],(uint8_t)args[3]);
      break;
    case SYS_lseek:
      args[0] = (uint64_t)f_lseek((FIL*)args[1],(uint32_t)args[2]);
      break;
    case SYS_mount:
      args[0] = (uint64_t)f_mount((FATFS*)args[1],(const TCHAR*)args[2],(uint8_t)args[3]);
      break;
    case SYS_unmount:
      args[0] = (uint64_t)f_mount(0,(const TCHAR*)args[2],(uint8_t)args[3]);
      break;
    case SYS_getcwd:
      args[0] = (uint64_t)f_getcwd((TCHAR*)args[1],(uint32_t)args[2]);
      break;
    case SYS_chdir:
      args[0] = (uint64_t)f_chdir((const TCHAR*)args[1]);
      break;
    case SYS_fcntl:
      args[0] = (uint64_t)sys_fcntl((FIL*)args[1],(int)args[2],(uint64_t)args[3]);
      break;
    case SYS_unlink:
      args[0] = (uint64_t)f_unlink((const TCHAR*)args[1]);
      break;
    case SYS_stat:
      args[0] = (uint64_t)sys_stat((FATFS*)args[1],(const TCHAR*)args[2],(struct stat *)args[3]);
      break;
    default:
      break;
  }
}
