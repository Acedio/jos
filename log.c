#include "log.h"

#include "fb.h"
#include "serial.h"
#include "string.h"

#ifdef LOG_TO_SCREEN
void (*log_puts)(const char*) = &fb_puts;
#else
void (*log_puts)(const char*) = &serial_puts;
#endif

void log_message_no_newline(int level, const char* filename, int line,
                            const char* text) {
  switch (level) {
    case INFO:
      log_puts("INFO:");
      break;
    case WARNING:
      log_puts("WARNING:");
      break;
    case ERROR:
      log_puts("ERROR:");
      break;
    default:
      log_puts("UNKNOWN:");
  }

  log_puts(filename);
  log_puts(":");
  char dec_str[12];
  int_to_dec(line, dec_str);
  log_puts(dec_str);
  log_puts(":");
  log_puts(text);
}

void log_message(int level, const char* filename, int line, const char* text) {
  log_message_no_newline(level, filename, line, text);
  log_puts("\n");
}

void log_int(int level, const char* filename, int line, const char* text,
             int i) {
  char dec[12];
  int_to_dec(i, dec);
  log_message_no_newline(level, filename, line, text);
  log_puts(dec);
  log_puts("\n");
}

void log_hex(int level, const char* filename, int line, const char* text,
             unsigned int i) {
  char hex[12];
  int_to_hex(i, hex);
  log_message_no_newline(level, filename, line, text);
  log_puts(hex);
  log_puts("\n");
}
