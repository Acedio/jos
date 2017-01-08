#include "paging.h"

#include "log.h"
#include "string.h"

// Defined in asm to get 4096 byte alignment
extern PageDirectoryEntry page_directory[1024];

void init_identity_paging() {
  for (unsigned int i = 0; i < 1024; ++i) {
    // Top 10 bits are the address of a naturally aligned 4MB, lower 12 are config bits, 21:12 are 0
    // bit 7 = PS, when set signifies a 4MB page frame
    // bit 6 = dirty (written)
    // bit 5 = accessed (read)
    // bit 1 = r/w
    // bit 0 = P, when set signifies presence of a frame or PDT
    page_directory[i] = (i << 22) | 0x83;
  }

  init_paging(page_directory);
}
