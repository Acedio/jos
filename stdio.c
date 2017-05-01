#include "stdio.h"

#include "fb.h"
#include "keyboard.h"
#include "log.h"

int fgetc(FILE* stream) {
  static int lshift = 0;
  static int rshift = 0;
  //static int ctrl = 0;
  //static int alt = 0;

  if (stream != stdin) {
    LOG_HEX(ERROR, "Unsupported stream: ", stream);
  }

  while (1) {
    while (!HasScancode()) {
      // Wait for input.
    }

    Scancode scancode = PopScancode();
    Key key = GetKey(scancode);

    if (KeyIsAscii(key)) { 
      if (IsPress(scancode)) {
        // Shift if necessary and return.
        return (lshift || rshift) ? ShiftedScancodeToAscii(scancode)
                                  : ScancodeToAscii(scancode);
      }
    } else if (key == KEY_LSHIFT) {
      // HACK: Note that the meta key handling won't handle releasing one key
      // after both left and right keys were held.
      lshift = IsPress(scancode);
    } else if (key == KEY_RSHIFT) {
      rshift = IsPress(scancode);
    } else if (key == KEY_LALT) {
      //alt = IsPress(scancode);
    } else if (key == KEY_LCTRL) {
      //ctrl = IsPress(scancode);
    }
  }

  return EOF;  // How did we get here? o_o
}

void fputc(int character, FILE* stream) {
  if (stream != stdout) {
    LOG_HEX(ERROR, "Unsupported stream: ", *stream);
  }
  fb_putchar((char)character);
}

int getc() { return fgetc(stdin); }
void putc(int character) { fputc(character, stdout); }
