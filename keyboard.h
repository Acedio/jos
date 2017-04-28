#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NONE 0xFF

// The data type used to store a scancode.
typedef unsigned char Key;

// Must be called before PushKey, PopKey, or HasKey.
void InitKeyboard();

// Pushes the current key into the ringbuf. Should only be called inside the
// keyboard interrupt handler.
void PushKey();
Key PopKey();
int HasKey();

typedef enum {
  KEY_TYPE_ASCII,
  KEY_TYPE_SHIFT,
  KEY_TYPE_ALT,
  KEY_TYPE_CTRL,
  KEY_TYPE_UNKNOWN
} KeyType;

char ShiftedKeyToAscii(Key key);
char KeyToAscii(Key key);

int IsPress(Key key);
KeyType GetKeyType(Key key);

char GetAscii();

#endif  // KEYBOARD_H
