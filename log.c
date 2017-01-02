#include "log.h"

#include "serial.h"

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

  // TODO: Write int_to_decimal()
  line += 1;

  serial_puts(filename);
  serial_puts(":");
  serial_puts(text);
  serial_puts("\n");
}
