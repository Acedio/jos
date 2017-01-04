#include "fb.h"
#include "interrupts.h"
#include "log.h"
#include "segmentation.h"
#include "serial.h"

int kmain() {
  serial_init();
  init_segmentation();
  init_interrupts();
  fb_set_color(15, 0);
  fb_clear();
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      fb_set_color(x, y);
      fb_write("Lol!", 4);
    }
    fb_write("\n", 1);
  }

  LOG(INFO, "help I'm trapped in a log factory.");

  return 0x42;
}
