#include "gdt.h"
#include "segmentation.h"

SegmentDescriptor segment_descriptors[3];

void init_segmentation() {
  // leave 0 (null) segment as is
  
  // segment 1 is for code
  segment_descriptors[1].base_31_24 = 0x00;
  // 0b1100 (4 byte granularity, 32-bit operations, no 64-bit operations)
  // 0b1111 (high nibble of segment limit)
  // 0b1001 (present in memory, privilege = 0, data/code segment
  // 0b1010 (execute/read, non-conforming, non-accessed)
  segment_descriptors[1].flags = 0xCF9A;
  segment_descriptors[1].base_23_16 = 0x00;
  segment_descriptors[1].base_15_0 = 0x0000;
  segment_descriptors[1].limit = 0xFFFF;

  // segment 2 is for data
  segment_descriptors[2].base_31_24 = 0x00;
  // 0b1100 (4 byte granularity, 32-bit operations, no 64-bit operations)
  // 0b1111 (high nibble of segment limit)
  // 0b1001 (present in memory, privilege = 0, data/code segment
  // 0b0010 (read/write, no-expand down [?], non-accessed)
  segment_descriptors[2].flags = 0xCF92;
  segment_descriptors[2].base_23_16 = 0x00;
  segment_descriptors[2].base_15_0 = 0x0000;
  segment_descriptors[2].limit = 0xFFFF;

  GDT gdt;
  gdt.address = (unsigned int)segment_descriptors;
  gdt.size = 24;

  lgdt(&gdt);
}
