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

typedef struct {
  FreePhysical* physical_page_stack_vaddr;
  FreePhysical* physical_page_stack_vtop;

  unsigned int* buddy_tree_vaddr;
  unsigned int  buddy_tree_depth;

  unsigned int  staging_vaddr;
  // index of the staging page table entry in the os page table
  unsigned int  staging_pte;
} MemCfg;

MemCfg mem_cfg;


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


void populate_free_physical(unsigned int start, unsigned int end,
                            FreePhysical* free_physical) {
  LOG(INFO, "Looks free!");
  LOG_HEX(INFO, " start: ", start);
  LOG_HEX(INFO, " end: ", end);
  free_physical->addr = start;
  free_physical->size = end - start;
}

// Finds a number (0<=n<=2) of free physical memory locations in mmap, while
// taking the kernel_location into account. Returns number of elements in
// free_physical that were populated.
int find_free_in_mmap(const memory_map_t* mmap, KernelLocation kernel_location,
                      FreePhysical free_physical[2]) {
  // 1 means available
  if (mmap->type == 1) {
    unsigned int start = mmap->base_addr_low;
    unsigned int end = mmap->base_addr_low + mmap->length_low;
    if (start <= kernel_location.physical_start) {
      if (end <= kernel_location.physical_start) {
        // Free range is completely before kernel.
        populate_free_physical(start, end, &free_physical[0]);
        return 1;
      } else if (end <= kernel_location.physical_end) {
        // Partial kernel collision (unless equal, then total collision).
        populate_free_physical(start, kernel_location.physical_end,
                               &free_physical[0]);
        return 1;
      } else {
        // Kernel is totally contained within this block.
        populate_free_physical(start, kernel_location.physical_start,
                               &free_physical[0]);
        populate_free_physical(kernel_location.physical_end, end,
                               &free_physical[1]);
        return 2;
      }
    } else if (start < kernel_location.physical_end) {
      if (end <= kernel_location.physical_end) {
        // Weird, the kernel covers this entire block?
        return 0;
      } else {
        // Partial kernel collision.
        populate_free_physical(kernel_location.physical_end, end,
                               &free_physical[0]);
        return 1;
      }
    } else {
      // Free range is completely after kernel.
      populate_free_physical(start, end, &free_physical[0]);
      return 1;
    }
  }
  return 0;
}

// Maps the 4kb virtual page at vaddr to the physical page at paddr, using the
// virtual page at staging_vaddr (index staging_pte in the OS's page table) as a
// staging area for updating the affected PT.
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

// Creates a stack of FreePhysical structs that map out the available physical
// memory of the system (which are determined using the passed mmap and
// kernel_location). mem_cfg->physical_page_stack_vaddr should be set to a free
// virtual page, and the OS's staging page should be set up as well before
// calling. Populates mem_cfg->physical_page_stack_vtop with the top address of
// the stack.
// TODO: This assumes that all inspected addresses fall on page boundaries,
// which is maybe not true?
void make_free_physical_stack(KernelLocation kernel_location,
                              unsigned long mmap_vaddr,
                              unsigned long mmap_length, MemCfg* mem_cfg) {
  mem_cfg->physical_page_stack_vtop = (FreePhysical*)0;

  LOG(INFO, "Populating free physical memory stack.");
  unsigned int i = 0;
  while (i < mmap_length) {
    memory_map_t* mmap = (memory_map_t*)(mmap_vaddr + i);
    FreePhysical free_physical[2];
    int num_free = find_free_in_mmap(mmap, kernel_location, free_physical);
    for (int i = 0; i < num_free; ++i ) {
      if (free_physical[i].size > 0) {
        // If we haven't yet allocated a page.
        if (!mem_cfg->physical_page_stack_vtop) {
          LOG_HEX(INFO, "Found free page at: ", free_physical[i].addr);
          map_page((unsigned int)mem_cfg->physical_page_stack_vaddr,
                   free_physical[i].addr, mem_cfg->staging_vaddr,
                   mem_cfg->staging_pte);
          mem_cfg->physical_page_stack_vtop =
              mem_cfg->physical_page_stack_vaddr;
          free_physical[i].addr += PAGE_SIZE;
          free_physical[i].size -= PAGE_SIZE;
          if (free_physical[i].size <= 0) {
            continue;
          }
        }
        *mem_cfg->physical_page_stack_vtop = free_physical[i];
        ++mem_cfg->physical_page_stack_vtop;
      }
    }
    i += mmap->size + 4;  // +4 because the size field does not include itself.
  }
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

  // Map the physical page stack to the virtual space after the end of the
  // kernel. It should be able to grow this way.
  mem_cfg.physical_page_stack_vaddr =
      (FreePhysical*)kernel_location.virtual_end;
  kernel_location.virtual_end += PAGE_SIZE;

  make_free_physical_stack(kernel_location, mmap_vaddr,
                           multiboot_info->mmap_length, &mem_cfg);
}
