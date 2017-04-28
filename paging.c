#include "paging.h"

#include "io.h"
#include "log.h"
#include "string.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)
#define PAGE_BITS 12

typedef unsigned int PageDirectoryEntry;
typedef unsigned int PageTableEntry;

// Defined in paging_asm.s to get 4096 byte alignment
extern PageDirectoryEntry page_directory[1024];
extern PageTableEntry os_page_table[1024];

typedef struct __attribute__((packed)) {
  unsigned int start;
  unsigned int end;
} MemorySpan;

typedef struct {
  MemorySpan* physical_page_stack_vaddr;
  MemorySpan* physical_page_stack_vtop;

  unsigned int  buddy_tree_vaddr;
  unsigned int  buddy_tree_depth;

  // REMEMBER TO INVALIDATE THE FRIGGIN' TLB WHEN CHANGING THE STAGING PAGE.
  unsigned int  staging_vaddr;
  // index of the staging page table entry in the os page table
  unsigned int  staging_pte;
} MemCfg;

MemCfg mem_cfg_;

typedef struct {
  unsigned int size;  // In bytes.
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

void make_span(unsigned int start, unsigned int end,
               MemorySpan* free_physical) {
  free_physical->start = start;
  free_physical->end = end;
}

// Finds a number (0<=n<=2) of free memory locations in the span from start to
// end, while taking the reserved_span into account. Returns number of elements
// in free_spans that were populated.
int find_free_with_reserved(unsigned int start, unsigned int end,
                            MemorySpan reserved_span,
                            MemorySpan free_spans[2]) {
  if (start <= reserved_span.start) {
    if (end <= reserved_span.start) {
      // Free range is completely before the reserved_span.
      make_span(start, end, &free_spans[0]);
      return 1;
    } else if (end <= reserved_span.end) {
      // Partial reserved_span collision (unless equal, then total collision).
      make_span(start, reserved_span.end, &free_spans[0]);
      return 1;
    } else {
      // reserved_span is totally contained within this block.
      make_span(start, reserved_span.start, &free_spans[0]);
      make_span(reserved_span.end, end, &free_spans[1]);
      return 2;
    }
  } else if (start < reserved_span.end) {
    if (end <= reserved_span.end) {
      // The reserved_span covers this entire block.
      return 0;
    } else {
      // Partial reserved_span collision.
      make_span(reserved_span.end, end, &free_spans[0]);
      return 1;
    }
  } else {
    // Free range is completely after reserved_span.
    make_span(start, end, &free_spans[0]);
    return 1;
  }
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
  invlpg(vaddr);
}

// Grabs the 4kb physical page currently associated with the given vaddr.
// Returns 0xFFFFFFFF on error.
unsigned int translate_vaddr(unsigned int vaddr, MemCfg* mem_cfg) {
  if (vaddr & PAGE_MASK) {
    LOG(ERROR, "Tried to map non-page virtual address.");
    return 0xFFFFFFFF;
  }
  unsigned int pde = (vaddr >> 22) & 0x3FF;
  if (!(page_directory[pde] & 1)) {
    LOG(ERROR, "Tried to translate virtual address that had no page table.");
    return 0xFFFFFFFF;
  }

  // Get the physical address of the page table.
  unsigned int pt_paddr = page_directory[pde] & 0xFFFFF000;
  // Map it into virtual memory in the OS's PT using the staging pte.
  os_page_table[mem_cfg->staging_pte] = pt_paddr | 0x3;
  invlpg(mem_cfg->staging_vaddr);

  PageTableEntry* pt = (PageTableEntry*)mem_cfg->staging_vaddr;
  unsigned int pte = (vaddr >> 12) & 0x3FF;
  if (!(pt[pte] & 1)) {
    // No page was mapped, return error.
    return 0xFFFFFFFF;
  }
  return pt[pte] & 0xFFFFF000;
}

// Pushes a span of memory defined by start and end onto the free physical
// memory stack. Reserves space for the stack if it doesn't yet exist.
void push_free_physical(unsigned int start, unsigned int end, MemCfg* mem_cfg) {
  if (!mem_cfg->physical_page_stack_vtop) {
    LOG_HEX(INFO, "First free physical page pushed. Found free page at: ",
            start);
    LOG_HEX(INFO, "                                       that ends at: ", end);
    map_page((unsigned int)mem_cfg->physical_page_stack_vaddr, start, mem_cfg);
    mem_cfg->physical_page_stack_vtop = mem_cfg->physical_page_stack_vaddr;
    start += PAGE_SIZE;
    if (start >= end) {
      return;
    }
  }
  LOG_HEX(INFO, "Pushing block starting at ", start);
  LOG_HEX(INFO, "                ending at ", end);
  make_span(start, end, mem_cfg->physical_page_stack_vtop);
  ++mem_cfg->physical_page_stack_vtop;
}

// Pushes spans of memory onto the stack of free physical memory, respecting the
// given reserved_spans by removing contained areas them from the span defined
// by [free_start, free_end) before pushing.
void push_free_physical_with_reserved(unsigned int free_start,
                                      unsigned int free_end,
                                      MemorySpan* reserved_spans,
                                      int num_reserved, MemCfg* mem_cfg) {
  LOG_HEX(INFO, "populate_physical_stack: free_start: ", free_start);
  LOG_HEX(INFO, "                           free_end: ", free_end);
  LOG_HEX(INFO, "                       num_reserved: ", num_reserved);
  if (free_start >= free_end) {
    return;
  }
  if (num_reserved <= 0) {
    // Base case: There are no reserved spans left, so we just push the whole
    // free span.
    push_free_physical(free_start, free_end, mem_cfg);
    return;
  }

  MemorySpan free_physical[2];
  // Find the physical blocks with respect to the first reserved span in the
  // list.
  int num_free = find_free_with_reserved(free_start, free_end, *reserved_spans,
                                         free_physical);
  for (int i = 0; i < num_free; ++i) {
    // Recurse to check the other reserved spans (there eventually will be no
    // more).
    push_free_physical_with_reserved(free_physical[i].start,
                                     free_physical[i].end, reserved_spans + 1,
                                     num_reserved - 1, mem_cfg);
  }
}

// Creates a stack of MemorySpan structs that map out the available physical
// memory of the system (which are determined using the passed mmap and
// kernel_location). mem_cfg->physical_page_stack_vaddr should be set to a free
// virtual page, and the OS's staging page should be set up as well before
// calling. Populates mem_cfg->physical_page_stack_vtop with the top address of
// the stack.
// TODO: This assumes that all inspected addresses fall on page boundaries,
// which is maybe not true?
void make_free_physical_stack(MemorySpan* reserved_spans,
                              int num_reserved,
                              unsigned long mmap_vaddr,
                              unsigned long mmap_length, MemCfg* mem_cfg) {
  // Set to null initially to specify that the stack needs to be allocated
  // physical space.
  mem_cfg->physical_page_stack_vtop = (MemorySpan*)0;

  LOG(INFO, "Populating free physical memory stack.");
  unsigned int i = 0;
  while (i < mmap_length) {
    memory_map_t* mmap = (memory_map_t*)(mmap_vaddr + i);
    // 1 means available
    if (mmap->type == 1) {
      unsigned int start = mmap->base_addr_low;
      unsigned int end = mmap->base_addr_low + mmap->length_low;
      push_free_physical_with_reserved(start, end, reserved_spans, num_reserved,
                                       mem_cfg);
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
  MemorySpan* free_physical = mem_cfg->physical_page_stack_vtop - 1;
  if (free_physical->end - free_physical->start < PAGE_SIZE) {
    LOG(ERROR, "Looks like a physical block wasn't page sized?");
    return 0;
  }
  unsigned int addr = free_physical->start;
  free_physical->start += PAGE_SIZE;
  if (free_physical->start >= free_physical->end) {
    // Pop off empty blocks.
    --mem_cfg->physical_page_stack_vtop;
  }
  return addr;
}

void push_physical(unsigned int mem, MemCfg* mem_cfg) {
  // TODO: Check for stack overflow.
  MemorySpan* free_physical = mem_cfg->physical_page_stack_vtop - 1;
  if (mem + PAGE_SIZE == free_physical->start) {
    // mem is at the beginning of the top chunk (somewhat likely).
    free_physical->start -= PAGE_SIZE;
  } else if (mem == free_physical->end) {
    // mem is at the end of the top chunk (pretty unlikely).
    free_physical->end += PAGE_SIZE;
  } else {
    // mem is not touching the top chunk (pretty likely).
    // So add a new free chunk.
    ++mem_cfg->physical_page_stack_vtop;
    free_physical = mem_cfg->physical_page_stack_vtop - 1;
    free_physical->start = mem;
    free_physical->end = mem + PAGE_SIZE;
  }
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

void free_buddy_index(unsigned int buddy_tree_vaddr, unsigned int buddy_index) {
  while (buddy_index && get_buddy_bit(buddy_tree_vaddr, buddy_index)) {
    // Mark the memory as free.
    set_buddy_bit(buddy_tree_vaddr, buddy_index, 0);
    // Check to see if the buddy is taken.
    if (get_buddy_bit(buddy_tree_vaddr, buddy_index ^ 1)) {
      // If it's taken, no further action is needed.
      return;
    }
    // If the buddy is also free, free the parent chunk.
    buddy_index >>= 1;
  }
}

void free_buddy_vaddr(unsigned int buddy_tree_vaddr, unsigned int freed_vaddr) {
  if (freed_vaddr & PAGE_MASK) {
    LOG_HEX(ERROR,
            "Tried to free non-page virtual address from the buddy tree: ",
            freed_vaddr);
    return;
  }
  // Start the index at the lowest level. buddy_index at 0 is not a valid value,
  // the root node is at index = 1.
  unsigned int buddy_index =
      (1 << (32 - PAGE_BITS)) + (freed_vaddr >> PAGE_BITS);
  if (!get_buddy_bit(buddy_tree_vaddr, buddy_index)) {
    LOG_HEX(ERROR, "freed_vaddr was already freed: ", freed_vaddr);
    return;
  }
  free_buddy_index(buddy_tree_vaddr, buddy_index);
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

// Finds and claims a block of VRAM of exactly 2^power_2 bytes and returns its
// address.
// BUG: This doesn't actually mark lower levels of the buddy tree, so if you
// try and claim a large block and then a small block it will allocate you
// overlapping areas of vmem.
unsigned int claim_vblock_of_power_2(unsigned int buddy_tree_vaddr,
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

// Finds a block of vram of at least the given size and marks it as claimed in
// the buddy tree. Returns the address of the VRAM and sets claimed_size (if
// non-null) to the claimed size.
unsigned int claim_vblock_of_size(unsigned int buddy_tree_vaddr,
                                  unsigned int requested_size,
                                  unsigned int* claimed_size) {
  // Round up to the next log2 e.g. 5 bytes -> 3
  int log2_roundup = log2(requested_size - 1) + 1;
  unsigned int mem = claim_vblock_of_power_2(buddy_tree_vaddr, log2_roundup);
  if (claimed_size) {
    *claimed_size = 1 << log2_roundup;
  }
  return mem;
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

// This always allocates in 4kb chunks. We throw a MemBlockInfo at the front of
// the allocated chunk to keep track of metadata.
void *malloc(unsigned int size) {
  unsigned int size_with_meminfo = size + sizeof(MemBlockInfo);
  // Check for overflow.
  if (size_with_meminfo < size) {
    return (void*)0;
  }
  if (size_with_meminfo < PAGE_SIZE) {
    size_with_meminfo = PAGE_SIZE;
  }
  unsigned int claimed_size;
  unsigned int mem = claim_vblock_of_size(mem_cfg_.buddy_tree_vaddr,
                                          size_with_meminfo, &claimed_size);
  // TODO: Handle the case of going over a page boundary (or multiple).
  add_page_table(mem, &mem_cfg_);
  for (unsigned int vaddr = mem; vaddr < mem + claimed_size;
       vaddr += PAGE_SIZE) {
    unsigned int paddr = pop_physical(&mem_cfg_);
    map_page(vaddr, paddr, &mem_cfg_);
  }
  MemBlockInfo* info = (MemBlockInfo*)mem;
  info->size = claimed_size;
  return (void*)(info + 1);
}

void free(void* mem) {
  MemBlockInfo* info = (MemBlockInfo*)mem - 1;
  if ((unsigned int)info & 0xFFF) {
    LOG_HEX(ERROR, "Tried to free() a non-page aligned chunk: ",
            (unsigned int)info);
    return;
  }
  for (unsigned int vaddr = (unsigned int)info;
       vaddr < (unsigned int)info + (1 << info->size); vaddr += PAGE_SIZE) {
    free_buddy_vaddr(mem_cfg_.buddy_tree_vaddr, vaddr);
    unsigned int paddr = translate_vaddr(vaddr, &mem_cfg_);
    // translate_vaddr returns 0xFFFFFFFF on error.
    if (paddr != 0xFFFFFFFF) {
      push_physical(paddr, &mem_cfg_);
    }
  }
}

unsigned int round_to_next_page(unsigned int addr) {
  return (addr + PAGE_SIZE - 1) & ~PAGE_MASK;
}

void init_paging(multiboot_info_t* multiboot_info,
                 KernelLocation kernel_location) {
  // TODO: Translate the physical address of the mmap to the virtual address in
  // a better way.
  unsigned long mmap_vaddr = multiboot_info->mmap_addr + KERNEL_VADDR;

  // First grab the staging page, which is used to populate page tables and
  // other things that we don't need to keep in the vaddr space. Round up to the
  // next page since the kernel end doesn't have to fall on a page boundary.
  mem_cfg_.staging_vaddr = round_to_next_page(kernel_location.virtual_end);
  mem_cfg_.staging_pte   = (mem_cfg_.staging_vaddr >> 12) & 0x3FF;
  kernel_location.virtual_end = mem_cfg_.staging_vaddr + PAGE_SIZE;

  // Map the physical page stack to the first virtual page after the end of the
  // kernel. It should be able to grow this way.
  // TODO: Seems like a good idea to just store this stack as a linked list in
  // the unused RAM itself.
  mem_cfg_.physical_page_stack_vaddr =
      (MemorySpan*)round_to_next_page(kernel_location.virtual_end);
  LOG_HEX(INFO, "page_stack_vaddr = ",
          (unsigned int)kernel_location.virtual_end);
  kernel_location.virtual_end =
      (unsigned int)mem_cfg_.physical_page_stack_vaddr + PAGE_SIZE;

  // Space for the kernal and the modules.
  MemorySpan reserved_blocks[1 + NUM_MODULES];

  // First the kernel.
  reserved_blocks[0].start = kernel_location.physical_start;
  reserved_blocks[0].end = kernel_location.physical_end;
  
  // Then the modules.
  if (multiboot_info->mods_count != NUM_MODULES) {
    LOG_HEX(ERROR, "Unexpected number of modules: ",
            multiboot_info->mods_count);
    return;
  }
  for (unsigned int i = 0; i < multiboot_info->mods_count; ++i) {
    module_t* module =
        (module_t*)(multiboot_info->mods_addr + KERNEL_VADDR) + i;
    reserved_blocks[i + 1].start = module->mod_start;
    // mod_start is page aligned, but mod_end isn't so we have to round up.
    reserved_blocks[i + 1].end = round_to_next_page(module->mod_end);
  }
  make_free_physical_stack(reserved_blocks,
                           NUM_MODULES + 1, mmap_vaddr,
                           multiboot_info->mmap_length, &mem_cfg_);
  make_virtual_buddy_tree(kernel_location, &mem_cfg_);
}

unsigned int map_module(module_t* module) {
  unsigned int mod_start = module->mod_start;
  // mod_start is page aligned, but mod_end isn't so we have to round up. 
  unsigned int mod_end = round_to_next_page(module->mod_end);
  LOG_HEX(INFO, "map_module: mod_start: ", mod_start);
  LOG_HEX(INFO, "              mod_end: ", mod_end);
  unsigned int module_vaddr =
      claim_vblock_of_size(mem_cfg_.buddy_tree_vaddr, mod_end - mod_start,
                           0 /* don't care about size */);
  add_page_table(module_vaddr, &mem_cfg_);
  unsigned int vaddr = module_vaddr;
  for (unsigned int paddr = mod_start; paddr < mod_end; paddr += PAGE_SIZE) {
    map_page(vaddr, paddr, &mem_cfg_);
    vaddr += PAGE_SIZE;
  }
  return module_vaddr;
}

// There needs to be a struct to keep track of process metadata:
//   - Page directory for the program (program code and data loaded at 0x0 and
//     OS pages pre-mapped at 0xC0000000).
//   - Stack location (starting at 0xBFFFFFFB and growing down).
//   - Future: Allocated heap pages and a way to determine which have free space
//     remaining in them.
