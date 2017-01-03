#ifndef GDT_H
#define GDT_H

typedef struct __attribute__((packed)) {
  unsigned short size;  // in bytes, not descriptors
  unsigned int address;
} GDT;

// This entire descriptor is stored little endian.
typedef struct __attribute__((packed)) {
  unsigned short limit;
  unsigned short base_15_0;
  unsigned char base_23_16;
  unsigned short flags;
  unsigned char base_31_24;
} SegmentDescriptor;

void lgdt(GDT* gdt);

#endif  // GDT_H
