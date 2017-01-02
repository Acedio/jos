#include "test.h"

#include <stdio.h>

void expect_true(int val, const char* valstr, const char* file, int line) {
  if (!val) {
    printf("FAILURE in %s:%d: %s should be true but was not.\n", file, line, valstr);
  }
}
