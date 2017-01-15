#include "paging.h"

#include "log.h"
#include "string.h"

#define PAGE_SIZE 4096

// Defined in paging_asm.s to get 4096 byte alignment
extern PageDirectoryEntry page_directory[1024];
extern PageTableEntry os_page_table[1024];

typedef struct __attribute__((packed)) {
  unsigned int addr;
  unsigned int size;
} FreePhysical;

struct {
  FreePhysical* physical_page_stack_vaddr;
  FreePhysical* physical_page_stack_vtop;

  unsigned int* buddy_tree_vaddr;
  unsigned int  buddy_tree_depth;

  unsigned int  staging_vaddr;
  // index of the staging page table entry in the os page table
  unsigned int  staging_pte;
} mem_cfg;

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
unsigned int first_free_physical_page(KernelLocation kernel_location,
                                      unsigned long mmap_vaddr,
                                      unsigned long mmap_length) {
  unsigned int address = 0;
  unsigned int i = 0;
  while (i < mmap_length) {
    memory_map_t* mmap = (memory_map_t*)(mmap_vaddr + i);
    // 1 means available
    if (mmap->type == 1) {
      address = mmap->base_addr_low;
      // Check for kernel conflict. If there's a conflict no conflict, return
      // the address.
      if (address < kernel_location.physical_start ||
          address >= kernel_location.physical_end) {
        return address;
      }
      // If there's a conflict and the kernel's end is within this block of
      // memory then use the first available page after the kernel. Otherwise,
      // continue searching.
      if (kernel_location.physical_end <
          (mmap->base_addr_low + mmap->length_low)) {
        return kernel_location.physical_end;
      }
    }
    i += mmap->size + 4;  // +4 because the size field does not include itself.
  }
  return 0xFFFFFFFF;
}

void push_range(unsigned int start, unsigned int end,
                FreePhysical** physical_stack_vtop) {
  // Don't push zero length blocks.
  if (start >= end) return;

  LOG(INFO, "Looks free!");
  LOG_HEX(INFO, " start: ", start);
  LOG_HEX(INFO, " end: ", end);
  FreePhysical* pushed = *physical_stack_vtop;
  pushed->addr = start;
  pushed->size = end - start;
  *physical_stack_vtop += 1;
}

// TODO: This currently runs over the first_free_physical_page.
void populate_free_physical(KernelLocation kernel_location,
                            unsigned long mmap_vaddr, unsigned long mmap_length,
                            FreePhysical** physical_stack_vtop) {
  LOG(INFO, "Populating free physical memory stack.");
  unsigned int i = 0;
  while (i < mmap_length) {
    memory_map_t* mmap = (memory_map_t*)(mmap_vaddr + i);
    unsigned int start = mmap->base_addr_low;
    unsigned int end = mmap->base_addr_low + mmap->length_low;
    // 1 means available
    if (mmap->type == 1) {
      if (start <= kernel_location.physical_start) {
        if (end <= kernel_location.physical_start) {
          // Free range is completely before kernel.
          push_range(start, end, physical_stack_vtop);
        } else if (end <= kernel_location.physical_end) {
          // Partial kernel collision (unless equal, then total collision).
          push_range(start, kernel_location.physical_end, physical_stack_vtop);
        } else {
          // Kernel is totally contained within this block.
          push_range(start, kernel_location.physical_start, physical_stack_vtop);
          push_range(kernel_location.physical_end, end, physical_stack_vtop);
        }
      } else if (start < kernel_location.physical_end) {
        if (end <= kernel_location.physical_end) {
          // Weird, the kernel covers this entire block?
        } else {
          // Partial kernel collision.
          push_range(kernel_location.physical_end, end, physical_stack_vtop);
        }
      } else {
        // Free range is completely after kernel.
        push_range(start, end, physical_stack_vtop);
      }
    }
    i += mmap->size + 4;  // +4 because the size field does not include itself.
  }
}

// We have a map of physical memory that's available (from the multiboot_info)
// we need to remove the areas of this that are occupied by the kernel
// We also have a map of the used virtual memory (page_directory)

void map_page(unsigned int vaddr, unsigned int paddr,
              unsigned int staging_vaddr, unsigned int staging_pte) {
  LOG_HEX(INFO, "Mapping virtual page: ", vaddr);
  LOG_HEX(INFO, "    to physical page: ", paddr);
  if (vaddr & 0xFFF) {
    LOG(ERROR, "Tried to map non-page virtual address.");
    return;
  }
  if (paddr & 0xFFF) {
    LOG(ERROR, "Tried to map non-page physical address.");
    return;
  }
  unsigned int pde = (vaddr >> 22) & 0x3FF;
  if (!(page_directory[pde] & 1)) {
    LOG(ERROR, "Tried to map virtual address that had no page table.");
  }

  // Get the physical address of the page table.
  unsigned int pt_paddr = page_directory[pde] & 0xFFFFFC00;
  // Map it into virtual memory in the OS's PT using the staging pde.
  os_page_table[staging_pte] = pt_paddr | 0x3;

  PageTableEntry* pt = (PageTableEntry*)staging_vaddr;
  unsigned int pte = (vaddr >> 12) & 0x3FF;
  pt[pte] = paddr | 0x3;  // 4kb page
}

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location) {
  // TODO: Translate the physical address of the mmap to the virtual address in
  // a better way.
  unsigned long mmap_vaddr = multiboot_info->mmap_addr + 0xC0000000;

  // First grab the staging page, which is used to populate page tables and
  // other things that we don't need to keep in the vaddr space.
  mem_cfg.staging_vaddr = kernel_location.virtual_end;
  mem_cfg.staging_pte   = (kernel_location.virtual_end >> 12) & 0x3FF;
  kernel_location.virtual_end += PAGE_SIZE;

  // Find a physical page for the physical page stack to be stored at.
  unsigned int physical_stack_paddr = first_free_physical_page(
      kernel_location, mmap_vaddr, multiboot_info->mmap_length);
  LOG_HEX(INFO, "Found free page at: ", physical_stack_paddr);

  // Map the physical page stack to the virtual space after the end of the
  // kernel. It should be able to grow this way.
  mem_cfg.physical_page_stack_vaddr =
      (FreePhysical*)kernel_location.virtual_end;
  kernel_location.virtual_end += PAGE_SIZE;
  mem_cfg.physical_page_stack_vtop = mem_cfg.physical_page_stack_vaddr;
  map_page((unsigned int)mem_cfg.physical_page_stack_vaddr,
           physical_stack_paddr, mem_cfg.staging_vaddr, mem_cfg.staging_pte);
  populate_free_physical(kernel_location, mmap_vaddr,
                         multiboot_info->mmap_length,
                         &mem_cfg.physical_page_stack_vtop);
}
