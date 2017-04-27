#ifndef STDIO_H
#define STDIO_H

typedef int FILE;

// Initialize these? :P
FILE __STDIN;
FILE __STDOUT;

#define stdin (&__STDIN)
#define stdout (&__STDOUT)
#define stderr (&__STDOUT)

#define EOF 256

int fgetc(FILE* stream);
void fputc(int character, FILE* stream);

int getc();
void putc(int character);

#endif  // STDIO_H
