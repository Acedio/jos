#include "paging.h"

#include "log.h"
#include "string.h"

// Defined in paging_asm.s to get 4096 byte alignment
extern PageDirectoryEntry page_directory[1024];
extern PageTableEntry os_page_table[1024];

// # Physical memory allocation
// Keep a stack of available memory pysical pages, along with their sizes in the
// last 12 bits.
// When a page is needed, check the top of the stack. Grab and decrement the
// size counter. Pop if zero.
// When a page is freed, compare its address with the top of the stack. If
// they're the same, increment the size (for the possibly common case of
// deallocating adjacent pages?).
// Figure that if we run out of stack space we can always add more pages to the
// stack (and free it of a page in the meantime :D ). We could also just drop
// the free'd pages on the floor and wait til we run out to invoke an expensive
// recollection routine to go through the virtual page tables to refresh the
// state of available pages. Thrashin'!
//
// # Virtual memory allocation
// Buddy allocation, which needs 128kb of memory to store the statuses of the
// finest layer (individual 4kb pages):
//   4 gigs is 1024 * 1024 4kb pages pages (20 address bits of pages)
//   Use each bit of a byte to store status (-3 bits, so 17 address bits/128kb)
// The subsequent layers need half the space of the layer below them, so another
// 128kb. All layers need 256kb. We'll store this in a heap structure:
//   { 0.0, 1.0, 1.1, 2.0, 2.1, 2.2, 2.3, 3.0, etc }
//
// # How do we get the space needed for these structures?
// First, we need the memory to store the physical stack. For the physical
// space, we can find a single free page based on the kernel location and
// multiboot info passed from the loader. For the virtual space we can use the
// first available page in the kernel's page table. We'll also write an
// expansion routine that wil allow us to add more frames to the physical page
// stack.
// Next, we should find the physical address space to use for the buddy system
// (18 address bits of space, 256kb). We can use the physical page stack to get
// this space. For the virtual space, When this is called I believe only the OS
// is using the virtual space, so we should be fine to just use the last 256kb
// of space (0xFFFC0000).

// Scan through the kernel location and mmap to find the first available
// physical page. Returns 0xFFFFFFFF if none was found.
// TODO: This assumes that all inspected addresses fall on page boundaries,
// which is maybe not true?
void* first_free_physical_page(KernelLocation kernel_location,
                               unsigned long mmap_addr,
                               unsigned long mmap_length) {
  unsigned int address = 0;
  unsigned int i = 0;
  while (i < mmap_length) {
    memory_map_t* mmap = (memory_map_t*)(mmap_addr + i);
    // 1 means available
    if (mmap->type == 1) {
      address = mmap->base_addr_low;
      // Check for kernel conflict. If there's a conflict no conflict, return
      // the address.
      if (address < kernel_location.kernel_physical_start ||
          address >= kernel_location.kernel_physical_end) {
        return (void*)address;
      }
      // If there's a conflict and the kernel's end is within this block of
      // memory then use the first available page after the kernel. Otherwise,
      // continue searching.
      if (kernel_location.kernel_physical_end <
          (mmap->base_addr_low + mmap->length_low)) {
        return (void*)kernel_location.kernel_physical_end;
      }
    }
    i += mmap->size + 4;  // +4 because the size field does not include itself.
  }
  return (void*)0xFFFFFFFF;
}

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location) {
  // TODO: Translate the physical address of the mmap to the virtual address in
  // a better way.
  LOG_HEX(INFO, "Found free page at: ",
          (unsigned long)first_free_physical_page(
              kernel_location, multiboot_info->mmap_addr + 0xC0000000,
              multiboot_info->mmap_length));
}

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
