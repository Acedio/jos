#ifndef SERIAL_H
#define SERIAL_H

void serial_init();

void serial_write(const char* buf, unsigned int len);

// Null terminated string.
void serial_puts(const char* str);

#endif  // SERIAL_H
