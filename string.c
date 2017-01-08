#include "string.h"

#define INT_MIN -2147483648

void int_to_dec(int i, char dec_str[12]) {
  int is_negative = i < 0;
  // Avoid undefined behavior by not trying to negate INT_MIN.
  if (is_negative && i != INT_MIN) {
    i = -i;
  }

  unsigned int num = i;

  // First we write the number from the back (lsd first)
  int pos = 11;
  // Null terminate it.
  dec_str[pos] = 0;
  --pos;
  while (num > 0) {
    dec_str[pos] = '0' + (num % 10);
    --pos;
    num /= 10;
  }
  if (is_negative) {
    dec_str[pos] = '-';
    --pos;
  }

  int shift = pos + 1;

  // Now shift it.
  for (pos = 0; pos < 12 - shift; ++pos) {
    dec_str[pos] = dec_str[pos + shift];
  }
}

// Leaving this at 12 so it's interoperable with int_to_dec.
void int_to_hex(unsigned int i, char hex_str[12]) {
  hex_str[0] = '0';
  hex_str[1] = 'x';
  hex_str[10] = 0;
  hex_str[11] = 0;
  for (int c = 9; c > 1; --c) {
    char hex_char;
    if ((i & 0xF) > 0x9) {
      hex_char = 'A' + ((i & 0xF) - 0xA);
    } else {
      hex_char = '0' + (i & 0xF);
    }
    hex_str[c] = hex_char;
    i >>= 4;
  }
}
