#include "paging.h"

#include "io.h"
#include "log.h"
#include "string.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)
#define PAGE_BITS 12

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

  unsigned int  buddy_tree_vaddr;
  unsigned int  buddy_tree_depth;

  // REMEMBER TO INVALIDATE THE FRIGGIN' TLB WHEN CHANGING THE STAGING PAGE.
  unsigned int  staging_vaddr;
  // index of the staging page table entry in the os page table
  unsigned int  staging_pte;
} MemCfg;

MemCfg mem_cfg;

typedef struct {
  unsigned int log2_size;
} MemBlockInfo;

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
void map_page(unsigned int vaddr, unsigned int paddr, MemCfg* mem_cfg) {
  LOG_HEX(INFO, "Mapping virtual page: ", vaddr);
  LOG_HEX(INFO, "    to physical page: ", paddr);
  if (vaddr & PAGE_MASK) {
    LOG(ERROR, "Tried to map non-page virtual address.");
    return;
  }
  if (paddr & PAGE_MASK) {
    LOG(ERROR, "Tried to map non-page physical address.");
    return;
  }
  unsigned int pde = (vaddr >> 22) & 0x3FF;
  if (!(page_directory[pde] & 1)) {
    LOG(ERROR, "Tried to map virtual address that had no page table.");
  }

  // Get the physical address of the page table.
  unsigned int pt_paddr = page_directory[pde] & 0xFFFFF000;
  // Map it into virtual memory in the OS's PT using the staging pte.
  os_page_table[mem_cfg->staging_pte] = pt_paddr | 0x3;
  invlpg(mem_cfg->staging_vaddr);

  PageTableEntry* pt = (PageTableEntry*)mem_cfg->staging_vaddr;
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
                   free_physical[i].addr, mem_cfg);
          mem_cfg->physical_page_stack_vtop =
              mem_cfg->physical_page_stack_vaddr;
          free_physical[i].addr += PAGE_SIZE;
          free_physical[i].size -= PAGE_SIZE;
          if (free_physical[i].size <= 0) {
            continue;
          }
        }
        LOG_HEX(INFO, "Pushing block at ", free_physical[i].addr);
        *mem_cfg->physical_page_stack_vtop = free_physical[i];
        ++mem_cfg->physical_page_stack_vtop;
      }
    }
    i += mmap->size + 4;  // +4 because the size field does not include itself.
  }
}

// Returns the address of a 4k page aligned chunk of memory.
unsigned int pop_physical(MemCfg* mem_cfg) {
  if (mem_cfg->physical_page_stack_vtop == mem_cfg->physical_page_stack_vaddr) {
    LOG(ERROR, "No physical memory blocks left on the stack!");
    return 0;
  }
  FreePhysical* free_physical = mem_cfg->physical_page_stack_vtop - 1;
  if (free_physical->size < PAGE_SIZE) {
    LOG(ERROR, "Looks like a physical block wasn't page sized?");
    return 0;
  }
  unsigned int addr = free_physical->addr;
  free_physical->size -= PAGE_SIZE;
  free_physical->addr += PAGE_SIZE;
  if (free_physical->size == 0) {
    // Pop off empty blocks.
    --mem_cfg->physical_page_stack_vtop;
  }
  return addr;
}

int get_buddy_bit(unsigned int buddy_tree_vaddr, unsigned int buddy_index) {
  unsigned char buddy_byte =
      ((unsigned char*)buddy_tree_vaddr)[buddy_index >> 3];
  return (buddy_byte >> (buddy_index & 0x7)) & 0x1;
}

void set_buddy_bit(unsigned int buddy_tree_vaddr, unsigned int buddy_index,
                   unsigned int bit_val) {
  unsigned char buddy_bit_in_byte = 0x1 << (buddy_index & 0x7);
  if (bit_val) {
    ((unsigned char*)buddy_tree_vaddr)[buddy_index >> 3] |= buddy_bit_in_byte;
  } else {
    ((unsigned char*)buddy_tree_vaddr)[buddy_index >> 3] &=
        (~buddy_bit_in_byte);
  }
}

void claim_buddy_index(unsigned int buddy_tree_vaddr,
                       unsigned int buddy_index) {
  // Reminder that buddy_index = 0 is not valid, and that the root is at index =
  // 1.
  while (buddy_index && !get_buddy_bit(buddy_tree_vaddr, buddy_index)) {
    set_buddy_bit(buddy_tree_vaddr, buddy_index, 1);
    // Go up to the parent next.
    buddy_index >>= 1;
  }
}

void claim_buddy_vaddr(unsigned int buddy_tree_vaddr,
                       unsigned int claimed_vaddr) {
  if (claimed_vaddr & PAGE_MASK) {
    LOG_HEX(ERROR,
            "Tried to claim non-page virtual address from the buddy tree: ",
            claimed_vaddr);
    return;
  }
  // Start the index at the lowest level. buddy_index at 0 is not a valid value,
  // the root node is at index = 1.
  unsigned int buddy_index =
      (1 << (32 - PAGE_BITS)) + (claimed_vaddr >> PAGE_BITS);
  if (get_buddy_bit(buddy_tree_vaddr, buddy_index)) {
    LOG_HEX(ERROR, "claimed_vaddr was already claimed: ", claimed_vaddr);
    return;
  }
  claim_buddy_index(buddy_tree_vaddr, buddy_index);
}

void zero_page(unsigned int vaddr) {
  LOG_HEX(INFO, "Zeroing: ", vaddr);
  unsigned int* zeroing_addr = (unsigned int*)vaddr;
  for (unsigned int i = 0; i < PAGE_SIZE / sizeof(unsigned int); ++i) {
    *zeroing_addr = 0;
    ++zeroing_addr;
  }
}

void make_virtual_buddy_tree(KernelLocation kernel_location, MemCfg* mem_cfg) {
  // TODO: This is gonna assume our kernel is tiny (<4MB) and that the buddy
  // tree will fit along side it in a 4MB page.
  mem_cfg->buddy_tree_vaddr = kernel_location.virtual_end;
  LOG_HEX(INFO, "buddy_tree_vaddr: ", mem_cfg->buddy_tree_vaddr);
  unsigned int buddy_tree_size_bytes = 0x40000;  // 256 kb
  kernel_location.virtual_end += buddy_tree_size_bytes;
  for (unsigned int vaddr = mem_cfg->buddy_tree_vaddr;
       vaddr < mem_cfg->buddy_tree_vaddr + buddy_tree_size_bytes;
       vaddr += PAGE_SIZE) {
    unsigned int paddr = pop_physical(mem_cfg);
    map_page(vaddr, paddr, mem_cfg);
    // Zero all the memory (zero means free)
    zero_page(vaddr);
  }
  // Now claim the kernel space.
  for (unsigned int vaddr = kernel_location.virtual_start;
       vaddr < kernel_location.virtual_end; vaddr += PAGE_SIZE) {
    claim_buddy_vaddr(mem_cfg->buddy_tree_vaddr, vaddr);
  }
}

// FWIW, I thought of this independently (:P), but formatting taken from
// http://graphics.stanford.edu/~seander/bithacks.html
int log2(unsigned int v) {
  if (v == 0) {
    return -1;
  }
  int r = 0;
  int a;
  r = (v > 0xFFFF) << 4; v >>= r;
  a = (v > 0xFF)   << 3; v >>= a; r |= a;
  a = (v > 0xF)    << 2; v >>= a; r |= a;
  a = (v > 0x3)    << 1; v >>= a; r |= a;
                                  r |= (v >> 1);
  return r;
}

unsigned int alloc_vblock_of_size(unsigned int buddy_tree_vaddr,
                                  unsigned int power_2) {
  if (power_2 < PAGE_BITS) {
    return 0;
  }
  // Find the row of the tree we're searching. Max log2 will be rounded up to 32
  // (meaning we need 4 gigs of memory, ha), resulting in row 1, the first row
  // of the tree.  Row 0 (buddy_index 0) is an invalid entry.
  int row = 33 - power_2;
  int row_root = 1 << (row - 1);
  for (int index = row_root; index < (row_root << 2); ++index) {
    if (!get_buddy_bit(buddy_tree_vaddr, index)) {
      claim_buddy_index(buddy_tree_vaddr, index);
      return (index - row_root) << power_2;
    }
  }
  return 0;  // Not available.
}

// Add a page table to the pd for the vaddr if it needs it.
void add_page_table(unsigned int vaddr, MemCfg* mem_cfg) {
  unsigned int pde = (vaddr >> 22) & 0x3FF;
  if (!(page_directory[pde] & 1)) {
    LOG(INFO,
        "Tried to map virtual address that had no page table. Mapping a new "
        "page table.");
    LOG_HEX(INFO, "Adding a page table for vaddr: ", vaddr);
    LOG_HEX(INFO, "  which is pde: : ", pde);
    // Find an unused physical chunk
    unsigned int paddr = pop_physical(mem_cfg);
    LOG_HEX(INFO, "Found paddr for new page table: ", paddr);
    LOG_HEX(INFO, " staging_pte: ", mem_cfg->staging_pte);
    LOG_HEX(INFO, " staging_vaddr: ", mem_cfg->staging_vaddr);
    // Map it into the staging area so we can...
    os_page_table[mem_cfg->staging_pte] = paddr | 0x3;
    invlpg(mem_cfg->staging_vaddr);
    // ... zero it.
    zero_page(mem_cfg->staging_vaddr);
    // Map it into the page directory.
    page_directory[pde] = paddr | 0x3;
  }
}

void *malloc(unsigned int size) {
  unsigned int size_with_meminfo = size + sizeof(MemBlockInfo);
  // Check for overflow.
  if (size_with_meminfo < size) {
    return (void*)0;
  }
  if (size_with_meminfo < PAGE_SIZE) {
    size_with_meminfo = PAGE_SIZE;
  }
  // Round up to the next log2 e.g. 5 bytes -> 3
  int log2_roundup = log2(size_with_meminfo - 1) + 1;
  unsigned int mem =
      alloc_vblock_of_size(mem_cfg.buddy_tree_vaddr, log2_roundup);
  // TODO: Handle cases of >=4MB blocks
  add_page_table(mem, &mem_cfg);
  for (unsigned int vaddr = mem; vaddr < mem + (1 << log2_roundup); vaddr +=
       PAGE_SIZE) {
    unsigned int paddr = pop_physical(&mem_cfg);
    map_page(vaddr, paddr, &mem_cfg);
  }
  MemBlockInfo* info = (MemBlockInfo*)mem;
  info->log2_size = log2_roundup;
  return (void*)(info + 1);
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
  make_virtual_buddy_tree(kernel_location, &mem_cfg);
}
