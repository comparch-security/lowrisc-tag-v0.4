#include <string.h>
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "file.h"
#include <fcntl.h>


void fwrite_test(){
  FATFS FatFs;
  FIL fil;
  uint8_t buffer[128] = "This is for BBL FILE_WRITE test. If you saw this message, your BBL file system works fine!";
  FRESULT fr;
  uint32_t br;
  uint32_t i;
  file_t * fp;
  
  f_mount(NULL,"/",1);

  if(f_mount(&FatFs,"/",1)) {
    uart_send_string("Fail to mount SD driver!");
  }
  
  fr = f_open(&fil, "test.txt", FA_WRITE|FA_CREATE_ALWAYS);
  if (fr) {
    uart_send_string("failed to open test.text!");
  }

  // fp = file_open("test.txt",O_WRONLY);
  // if (!fp) {
  //   uart_send_string("failed to open test.text!");
  // }

  // filp = &fp->fd;

  uint32_t bw = 0;
  // bw = file_write(fp,buffer,strlen((char*)buffer));
  fr = f_write(&fil,buffer,strlen((char*)buffer),&bw);

  if (f_close(&fil)) {
    uart_send_string("fail to close file!");
  }

  if (f_mount(NULL,"",1)) {
    uart_send_string("fail to umount disk!");
  }
  
  uart_send_string("finished.\n");

}
