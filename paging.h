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

void* malloc(unsigned int size);
void free(void* mem);

unsigned int map_module(module_t* module);

#endif  // PAGING_H
