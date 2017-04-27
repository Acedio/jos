#include "stdio.h"

#include "fb.h"
#include "keyboard.h"
#include "log.h"

int fgetc(FILE* stream) {
  static int shift = 0;
  //static int ctrl = 0;
  //static int alt = 0;

  if (stream != stdin) {
    LOG_HEX(ERROR, "Unsupported stream: ", (unsigned int)stream);
  }

  while (1) {
    while (!HasKey()) {
      // Wait for input.
    }

    Key key = PopKey();
    KeyType type = GetKeyType(key);

    if (type == KEY_TYPE_ASCII) { 
      if (IsPress(key)) {
        // Convert to lowercase if necessary and return.
        return KeyToAscii(key) + (shift ? 0 : ('a' - 'A'));
      }
    } else if (type == KEY_TYPE_SHIFT) {
      // HACK: Note that the meta key handling won't handle releasing one key
      // after both left and right keys were held.
      shift = IsPress(key);
    } else if (type == KEY_TYPE_ALT) {
      //alt = IsPress(key);
    } else if (type == KEY_TYPE_CTRL) {
      //ctrl = IsPress(key);
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
