#include "fb.h"

int cfunc(int arg1, int arg2, int arg3) {
  fb_set_color(11, 13);
  fb_clear();
  unsigned char color = 0;
  while (1) {
    fb_set_color(color >> 4, color & 0xF);
    fb_write("lol!", 4);
    color += 1;
  }

  return arg1 + 2 * arg2 + 3 * arg3;
}
