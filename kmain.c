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
    "|/    v0.1\n";
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

int kmain(multiboot_info_t* multiboot) {
  serial_init();
  init_segmentation();
  init_interrupts();
  fb_set_color(15, 0);
  fb_clear();
  logo();
  fb_set_color(7, 0);
  fb_puts("\n\njosh@jos $ ");

  LOG_HEX(INFO, "Flags: ", multiboot->flags);

  LOG(INFO, "help I'm trapped in a log factory.");

  return 0x42;
}
