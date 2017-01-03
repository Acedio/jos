#include "segmentation.h"

#include "gdt.h"
#include "log.h"
#include "string.h"

SegmentDescriptor gdt[3];

void init_segmentation() {
  // leave 0 (null) segment as is
  
  // segment 1 is for code
  gdt[1].base_31_24 = 0x00;
  // 0b1100 (4 byte granularity, 32-bit operations, no 64-bit operations)
  // 0b1111 (high nibble of segment limit)
  // 0b1001 (present in memory, privilege = 0, data/code segment
  // 0b1010 (execute/read, non-conforming, non-accessed)
  gdt[1].flags = 0xCF9A;
  gdt[1].base_23_16 = 0x00;
  gdt[1].base_15_0 = 0x0000;
  gdt[1].limit = 0xFFFF;

  // segment 2 is for data
  gdt[2].base_31_24 = 0x00;
  // 0b1100 (4 byte granularity, 32-bit operations, no 64-bit operations)
  // 0b1111 (high nibble of segment limit)
  // 0b1001 (present in memory, privilege = 0, data/code segment
  // 0b0010 (read/write, no-expand down [?], non-accessed)
  gdt[2].flags = 0xCF92;
  gdt[2].base_23_16 = 0x00;
  gdt[2].base_15_0 = 0x0000;
  gdt[2].limit = 0xFFFF;

  GDTSpec gdt_spec;
  gdt_spec.address = (unsigned int)gdt;
  gdt_spec.size = sizeof(gdt);

  lgdt(&gdt_spec);
}
