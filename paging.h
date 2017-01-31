#ifndef PAGING_H
#define PAGING_H

#include "multiboot.h"

#define NUM_MODULES 1

typedef struct __attribute__((packed)) {
  unsigned int physical_start;
  unsigned int physical_end;
  unsigned int virtual_start;
  unsigned int virtual_end;
} KernelLocation;

typedef unsigned int PageDirectoryEntry;
typedef unsigned int PageTableEntry;

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location);

void* malloc(unsigned int size);
void free(void* mem);

#endif  // PAGING_H
