#include "fb.h"

int kmain() {
  fb_set_color(15, 0);
  fb_clear();
  char str[] =
    "1 hello, how are you?\n"
    "2 this is quite an auspicious day.\n"
    "3 new years eve, in fact!\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "24\n"
    "25\n"
    "26 combo breaker!";
  fb_write(str, sizeof(str));

  return 0x42;
}
