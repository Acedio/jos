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
