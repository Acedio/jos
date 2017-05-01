#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_SCANCODE 0xFF

// The data type used to store a scancode.
typedef unsigned char Scancode;

// Must be called before PushKey, PopKey, or HasKey.
void InitKeyboard();

// Pushes the current scancode into the ringbuf. Should only be called inside
// the keyboard interrupt handler.
void PushScancode();
Scancode PopScancode();
int HasScancode();

typedef enum {
  KEY_UNKNOWN = 0,
  // (0, 256) are equal to their ASCII equivalent.
  KEY_LSHIFT = 256,
  KEY_RSHIFT,
  KEY_LALT,
  KEY_RALT,
  KEY_LCTRL,
  KEY_RCTRL
} Key;

char ScancodeToAscii(Scancode Scancode);
char ShiftedScancodeToAscii(Scancode Scancode);

int IsPress(Scancode Scancode);
Key GetKey(Scancode Scancode);
int KeyIsAscii(Key key);

char GetAscii();

#endif  // KEYBOARD_H
