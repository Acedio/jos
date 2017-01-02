#include "fb.h"
#include "serial.h"
#include "log.h"

int kmain() {
  fb_set_color(15, 0);
  fb_clear();
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      fb_set_color(x, y);
      fb_write("Lol!", 4);
    }
    fb_write("\n", 1);
  }

  serial_init();
  serial_write("lol!!!", 6);
  LOG(INFO, "help I'm trapped in a log factory.");

  return 0x42;
}
