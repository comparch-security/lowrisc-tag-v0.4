// See LICENSE for license details.

// just jump from BRAM to DDR

#include <stdio.h>
#include <stdlib.h>
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "elf.h"
#include "encoding.h"
#include "bits.h"
#include "memory.h"
#include "spi.h"
#include "bio.h"

// max size of file image is 64M
#define MAX_FILE_SIZE 0x4000000

FATFS FatFs;   /* Work area (file system object) for logical drive */

void jump_to_ddr (void)
{
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}

int main (void)
{
  uint8_t *bram_base   = (uint8_t *)(get_bram_base());
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  uint8_t *buffer  = bram_base + get_bram_size() - 128;
  uint8_t *bbl_buf = memory_base + get_ddr_size() - MAX_FILE_SIZE; // at the end of DDR space

  FIL fil;                /* File object */
  FRESULT fr;             /* FatFs return code */
  uint32_t br;            /* Read count */
  uint32_t i;

  uart_init();

  /* mount the SD drive */
  if(f_mount(&FatFs, "", 1)) {
    printf("Fail to mount SD driver!\n");
    return 1;
  }

  /* get the current case number */
  int current_case = 0;
  if (f_open(&fil, "current.txt", FA_READ|FA_WRITE)) { // not found, create one
    if(f_open(&fil, "current.txt", FA_CREATE_NEW|FA_READ|FA_WRITE)) {
      printf("Can't read or create current.txt!");
      return 1;
    } else {
      f_puts("1", &fil);
      f_close(&fil);
    }
  } else {
    if(NULL == f_gets(buffer, 64, &fil)) {
      printf("Fail to read current.txt!");
      f_close(&fil);
      return 1;
    }
    current_case = atoi(buffer);
    itoa(current_case+1, buffer, 10);
    f_rewind(&fil);
    f_puts(buffer, &fil);
    f_close(&fil);
  }
  printf("Current test case is %d\n", current_case);
  set_led(current_case);

  // read the test case
  if (f_open(&fil, "cases.txt", FA_READ)) { // not found
    printf("Can't open cases.txt!");
    return 1;
  }
  for(i=0; i<current_case+1; i++) {
    if(NULL == f_gets(buffer, 128, &fil)) {
      printf("Fail to read the current case from cases.txt!");
      return 1;
    }
  }
  if(f_close(&fil)) {
    printf("fail to close cases.txt!");
    return 2;
  }
  printf("  %s\n", buffer);

  // load bbl
  printf("Load bbl into memory\n");

  if(f_open(&fil, "bbl", FA_READ)) {
    printf("Failed to open bbl!\n");
    return 2;
  } else {
    printf("Size of the bbl is %ld bytes.\n", f_size(&fil));
    if(f_size(&fil) >= MAX_FILE_SIZE) {
      printf("bbl is too large to be loaded! (maximal %ld MB)\n", MAX_FILE_SIZE >> 20);
      return 3;
    }
  }

  // the the content of bbl
  if(f_read(&fil, bbl_buf, MAX_FILE_SIZE, &br) || br != f_size(&fil)) {
    printf("Failed to read the full bbl! Only %ld bytes are read in the total of %ld bytes.\n", br, f_size(&fil));
    return 2;
  }
  if(f_close(&fil)) {
    printf("fail to close bbl!");
    return 2;
  }

  // read elf
  printf("load elf to DDR memory\n");
  if(load_elf(bbl_buf, br)) {
    printf("elf read failed");
    return 2;
  }

  // umount the SD drive
  if(f_mount(NULL, "", 0)) {         // unmount it
    printf("fail to umount disk!");
    return 1;
  }
  spi_disable();

  printf("Boot bbl...\n");
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}
