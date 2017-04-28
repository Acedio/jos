#include "fb.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "log.h"
#include "multiboot.h"
#include "paging.h"
#include "segmentation.h"
#include "serial.h"
#include "stdio.h"

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

void log_multiboot(multiboot_info_t* multiboot) {
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

void log_kernel_location(KernelLocation kernel_location) {
  LOG_HEX(INFO, "kernel.physical_start: ", kernel_location.physical_start);
  LOG_HEX(INFO, "kernel.physical_end: ", kernel_location.physical_end);
  LOG_HEX(INFO, "kernel.virtual_start: ", kernel_location.virtual_start);
  LOG_HEX(INFO, "kernel.virtual_end: ", kernel_location.virtual_end);
}

void test_malloc() {
  unsigned int* a = (unsigned int*)malloc(4096 * 20);
  LOG_HEX(INFO, "malloc'd vaddr a = ", a);
  unsigned int* b = (unsigned int*)malloc(2 * sizeof(unsigned int));
  LOG_HEX(INFO, "malloc'd vaddr b = ", b);
  free(a);
  LOG(INFO, "free'd a");
  unsigned int* c = (unsigned int*)malloc(2 * sizeof(unsigned int));
  LOG_HEX(INFO, "malloc'd vaddr c = ", a);
  free(c);
  LOG(INFO, "free'd c");
  free(b);
  LOG(INFO, "free'd b");
}

int kmain(multiboot_info_t* multiboot, KernelLocation kernel_location) {
  serial_init();
  init_segmentation();
  init_interrupts();
  log_multiboot(multiboot);
  log_kernel_location(kernel_location);
  init_paging(multiboot, kernel_location);
  InitKeyboard();
  fb_set_color(15, 0);
  fb_clear();
  logo();
  fb_set_color(7, 0);
  fb_puts("\n\njosh@jos $ ");

  LOG(INFO, "help I'm trapped in a log factory.");

  test_malloc();

  if (multiboot->mods_count != 1) {
    LOG_HEX(ERROR, "Unexpected number of modules: ", multiboot->mods_count);
    return -1;
  }

  module_t* module = (module_t*)(multiboot->mods_addr + 0xC0000000);

  unsigned int (*program)(void) = (unsigned int (*)(void))map_module(module);
  LOG_HEX(INFO, "HERE WE GO, INTO YONDER USER PROGRAM! ",
          (unsigned int)program);
  if (module->string) {
    LOG(INFO, (char*)(module->string + 0xC0000000));
  }
  unsigned int result = program();
  LOG_HEX(INFO, "program result = ", result);

  while (1) {
    putc(getc());
  }

  return 0;
}
