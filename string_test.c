#include <string.h>

#include "string.h"
#include "test.h"

int main() {
  char dec_str[12];
  int_to_dec(-123, dec_str);
  EXPECT_TRUE(strcmp("-123", dec_str) == 0);
  int_to_dec(2147483647, dec_str);
  EXPECT_TRUE(strcmp("2147483647", dec_str) == 0);
  int_to_dec(-2147483648, dec_str);
  EXPECT_TRUE(strcmp("-2147483648", dec_str) == 0);

  int_to_hex(0xCAFEBABE, dec_str);
  EXPECT_TRUE(strcmp("0xCAFEBABE", dec_str) == 0);
  return 0;
}
