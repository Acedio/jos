#include "log.h"

#include "serial.h"
#include "string.h"

void log_message(int level, const char* filename, int line, const char* text) {
  switch (level) {
    case INFO:
      serial_puts("INFO:");
      break;
    case WARNING:
      serial_puts("WARNING:");
      break;
    case ERROR:
      serial_puts("ERROR:");
      break;
    default:
      serial_puts("UNKNOWN:");
  }

  serial_puts(filename);
  serial_puts(":");
  char dec_str[12];
  int_to_dec(line, dec_str);
  serial_puts(dec_str);
  serial_puts(":");
  serial_puts(text);
  serial_puts("\n");
}
