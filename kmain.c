#include "fb.h"
#include "interrupts.h"
#include "log.h"
#include "paging.h"
#include "segmentation.h"
#include "serial.h"

void logo() {
  fb_puts("Welcome to\n");
  fb_puts("   ,--.---.\n");
  fb_puts(",-|   | __|\n");
  fb_puts("`_| O |__ |\n");
  fb_puts("| |___|___|\n");
  fb_puts("| |\n");
  fb_puts("|/\n");
}

int kmain() {
  serial_init();
  init_segmentation();
  init_interrupts();
  init_identity_paging();
  fb_set_color(15, 0);
  fb_clear();
  fb_set_color(13, 0);
  logo();
  fb_set_color(7, 0);
  fb_puts("\n\njosh@jos $ ");

  LOG(INFO, "help I'm trapped in a log factory.");

  return 0x42;
}
