#include "fb.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "multiboot.h"
#include "paging.h"
#include "segmentation.h"
#include "serial.h"

void logo() {
  const char* logo_str =
    "Welcome to\n"
    "   ,--.---.\n"
    ",-|   | __|\n"
    "`_| O |__ |\n"
    "| |___|___|\n"
    "| |\n"
    "|/  v0.0.1\n";
  unsigned char colors[] = {4, 6, 14, 10, 3, 1, 5};
  unsigned int color = 0;
  unsigned int color_start = 0;
  unsigned int i = 0;
  while (logo_str[i]) {
    fb_set_color(colors[color], 0);
    fb_putchar(logo_str[i]);
    ++color;
    if (color >= sizeof(colors)) {
      color = 0;
    }
    if (logo_str[i] == '\n') {
      if (color_start == 0) {
        color_start = sizeof(colors) - 1;
      } else {
        --color_start;
      }
      color = color_start;
    }
    ++i;
  }
}

void print_stats(multiboot_info_t* multiboot) {
  LOG_HEX(INFO, "FLAGS: ", multiboot->flags);
  if (multiboot->flags & 0x1) {
    LOG_HEX(INFO, "MEM_UPPER: ", multiboot->mem_upper);
    LOG_HEX(INFO, "MEM_LOWER: ", multiboot->mem_lower);
  }
  if (multiboot->flags & 0x40) {
    LOG_HEX(INFO, "MMAP_LENGTH: ", multiboot->mmap_length);
    LOG_HEX(INFO, "MMAP_ADDR: ", multiboot->mmap_addr);
    unsigned int i = 0;
    while (i < multiboot->mmap_length) {
      memory_map_t* addr = (memory_map_t*)(multiboot->mmap_addr + 0xC0000000 + i);
      LOG_HEX(INFO, "size: ", addr->size);
      LOG_HEX(INFO, "base_addr_low: ", addr->base_addr_low);
      LOG_HEX(INFO, "base_addr_high: ", addr->base_addr_high);
      LOG_HEX(INFO, "length_low: ", addr->length_low);
      LOG_HEX(INFO, "length_high: ", addr->length_high);
      LOG_HEX(INFO, "type: ", addr->type);
      i += addr->size + 4;  // +4 because the size field does not include itself.
    }
  }
}

int kmain(multiboot_info_t* multiboot, KernelLocation kernel_location) {
  serial_init();
  init_segmentation();
  init_interrupts();
  init_paging(multiboot, kernel_location);
  fb_set_color(15, 0);
  fb_clear();
  logo();
  fb_set_color(7, 0);
  fb_puts("\n\njosh@jos $ ");

  LOG_HEX(INFO, "kernel.physical_start: ", kernel_location.physical_start);
  LOG_HEX(INFO, "kernel.physical_end: ", kernel_location.physical_end);
  LOG_HEX(INFO, "kernel.virtual_start: ", kernel_location.virtual_start);
  LOG_HEX(INFO, "kernel.virtual_end: ", kernel_location.virtual_end);

  LOG(INFO, "help I'm trapped in a log factory.");

  print_stats(multiboot);

  unsigned int* i = (unsigned int*)malloc(2 * sizeof(unsigned int));
  LOG_HEX(INFO, "malloc'd vaddr = ", (unsigned int)i);
  i = (unsigned int*)malloc(2 * sizeof(unsigned int));
  LOG_HEX(INFO, "malloc'd vaddr = ", (unsigned int)i);

  return 0x42;
}
