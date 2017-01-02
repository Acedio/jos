#ifndef LOG_H
#define LOG_H

#define INFO 2
#define WARNING 1
#define ERROR 0

#define LOG(level, text) log_message(level, __FILE__, __LINE__, text)

// Must call serial_init() first.
void log_message(int level, const char* filename, int line, const char* text);

#endif  // LOG_H
