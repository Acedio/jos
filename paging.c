#include "paging.h"

#include "log.h"
#include "string.h"

// Defined in paging_asm.s to get 4096 byte alignment
extern PageDirectoryEntry page_directory[1024];
extern PageTableEntry os_page_table[1024];

/*
void init_paging(KernelLocation kernel_location, memory_map_t* memory_map) {
}
*/

// We have a map of physical memory that's available (from the multiboot_info)
// we need to remove the areas of this that are occupied by the kernel
// We also have a map of the used virtual memory (page_directory)

void map_page(unsigned int virt, unsigned int physical) {
  if (virt & 0xFFF) {
    LOG(ERROR, "Tried to map non-page virtual address.");
    return;
  }
  if (physical & 0xFFF) {
    LOG(ERROR, "Tried to map non-page physical address.");
    return;
  }
}
