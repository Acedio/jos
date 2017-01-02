#ifndef TEST_H
#define TEST_H

#define EXPECT_TRUE(val) expect_true(val, #val, __FILE__, __LINE__)

void expect_true(int val, const char* valstr, const char* file, int line);

#endif  // TEST_H
