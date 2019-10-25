// See LICENSE for license details.

#include <iostream>
#include "htif_hexwriter.h"
#include "memif.h"
#include "elfloader.h"

int main(int argc, char** argv)
{
  if(argc < 4 || argc > 5)
  {
    std::cerr << "Usage: " << argv[0] << " [base] width depth elf_file" << std::endl;
    return 1;
  }

  unsigned int argc_offset = argc - 4;

  unsigned int base = 0;
  if(argc_offset) {
    base = atoi(argv[1]);
    if((base & (base-1)))
    {
      std::cerr << "base must be a power of 2" << std::endl;
      return 1;
    }
  }

  unsigned width = atoi(argv[1 + argc_offset]);
  if(width < 4 || (width & (width-1)))
  {
    std::cerr << "width must be at least 4 and a power of 2" << std::endl;
    return 1;
  }

  unsigned depth = atoi(argv[2 + argc_offset]);
  if(depth == 0 || (depth & (depth-1)))
  {
    std::cerr << "depth must be nonzero and a power of 2" << std::endl;
    return 1;
  }

  htif_hexwriter_t htif(base, width, depth);
  memif_t memif(&htif);
  load_elf(argv[3 + argc_offset], &memif);
  std::cout << htif;

  return 0;
}
