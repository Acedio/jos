#ifndef PAGING_H
#define PAGING_H

#include "multiboot.h"

#define NUM_MODULES 1
#define KERNEL_VADDR 0xC0000000

typedef struct __attribute__((packed)) {
  unsigned int physical_start;
  unsigned int physical_end;
  unsigned int virtual_start;
  unsigned int virtual_end;
} KernelLocation;

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location);

// Allocates a block of at least "size" bytes. Currently will allocate entire
// 4kb pages at a time (note: the first few bytes of the 4kb are used for
// bookkeeping), and doesn't support malloc'ing >4MB blocks.
// TODO: We should find a way of keeping track of recently allocated pages for
// per program so we can keep taking from the same page while it contains free
// space.
void* malloc(unsigned int size);

// Free's a previously malloc'd chunk of memory.
void free(void* mem);

unsigned int map_module(module_t* module);

#endif  // PAGING_H
