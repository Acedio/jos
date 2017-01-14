#ifndef PAGING_H
#define PAGING_H

#include "multiboot.h"

typedef struct __attribute__((packed)) {
  unsigned int kernel_physical_start;
  unsigned int kernel_physical_end;
  unsigned int kernel_virtual_start;
  unsigned int kernel_virtual_end;
} KernelLocation;

typedef unsigned int PageDirectoryEntry;
typedef unsigned int PageTableEntry;

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location);

void map_page(unsigned int virt, unsigned int physical);

#endif  // PAGING_H
