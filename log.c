#include "log.h"

#include "serial.h"
#include "string.h"

void log_message_no_newline(int level, const char* filename, int line, const char* text) {
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
}

void log_message(int level, const char* filename, int line, const char* text) {
  log_message_no_newline(level, filename, line, text);
  serial_puts("\n");
}

void log_int(int level, const char* filename, int line, const char* text,
             int i) {
  char dec[12];
  int_to_dec(i, dec);
  log_message_no_newline(level, filename, line, text);
  serial_puts(dec);
  serial_puts("\n");
}

void log_hex(int level, const char* filename, int line, const char* text,
             unsigned int i) {
  char hex[12];
  int_to_hex(i, hex);
  log_message_no_newline(level, filename, line, text);
  serial_puts(hex);
  serial_puts("\n");
}
