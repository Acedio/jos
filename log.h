#ifndef LOG_H
#define LOG_H

#define INFO 2
#define WARNING 1
#define ERROR 0

#define LOG(level, text) log_message(level, __FILE__, __LINE__, text)

#define LOG_INT(level, text, i) log_int(level, __FILE__, __LINE__, text, i)
#define LOG_HEX(level, text, i) log_hex(level, __FILE__, __LINE__, text, i)

// Must call serial_init() before calling any of these functions.
void log_message(int level, const char* filename, int line, const char* text);

void log_int(int level, const char* filename, int line, const char* text, int i);
void log_hex(int level, const char* filename, int line, const char* text,
             unsigned int i);

#endif  // LOG_H
