#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

#define ASSERT(e) do {if(!(e)) fprintf(stderr, "ASSERT failed: %s:%d: %s\n", __FILE__, __LINE__, #e); exit(1);}while(0)
#define ASSERT_EQUAL(e1, e2) \
do { \
  if((e1)!=(e2)) { \
    fprintf(stderr, "ASSERT_EQUAL failed: %s:%d: [%s] == [%s]\n", __FILE__, __LINE__, #e1, #e2);\
    exit(1);\
  }\
}while(0)

#define TEST(f) {#f, f}

typedef void (*TestFunc)();

struct Test {
  const char* name;
  TestFunc run;
};

void test_xsg_xug() {
  struct timeval x = {2, 2};
  struct timeval y = {1, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 1);
  ASSERT_EQUAL(d.tv_usec, 1);
}

void test_xse_xug() {
  struct timeval x = {1, 2};
  struct timeval y = {1, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 0);
  ASSERT_EQUAL(d.tv_usec, 1);
}

void test_xsl_xug() {
  struct timeval x = {1, 2};
  struct timeval y = {2, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 0);
  ASSERT_EQUAL(d.tv_usec, -999999);
}

void test_xsg_xue() {
  struct timeval x = {2, 1};
  struct timeval y = {1, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 1);
  ASSERT_EQUAL(d.tv_usec, 0);
}

void test_xse_xue() {
  struct timeval x = {1, 1};
  struct timeval y = {1, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 0);
  ASSERT_EQUAL(d.tv_usec, 0);
}

void test_xsl_xue() {
  struct timeval x = {1, 1};
  struct timeval y = {2, 1};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, -1);
  ASSERT_EQUAL(d.tv_usec, 0);
}

void test_xsg_xul() {
  struct timeval x = {2, 1};
  struct timeval y = {1, 2};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 0);
  ASSERT_EQUAL(d.tv_usec, 999999);
}

void test_xse_xul() {
  struct timeval x = {1, 1};
  struct timeval y = {1, 2};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, 0);
  ASSERT_EQUAL(d.tv_usec, -1);
}

void test_xsl_xul() {
  struct timeval x = {1, 1};
  struct timeval y = {2, 2};
  struct timeval d = time_sub(&x, &y);
  ASSERT_EQUAL(d.tv_sec, -1);
  ASSERT_EQUAL(d.tv_usec, -1);
}

static struct Test tests[] = {
  TEST(test_xsg_xug),
  TEST(test_xse_xug),
  TEST(test_xsl_xug),
  TEST(test_xsg_xue),
  TEST(test_xse_xue),
  TEST(test_xsg_xue),
  TEST(test_xsg_xul),
  TEST(test_xse_xul),
  TEST(test_xsl_xul),
};
static const int num_tests = sizeof(tests) / sizeof(tests[0]);

int main(int argc, char** argv) {
  int i;
  for(i = 0; i < num_tests; ++i) {
    struct Test* t = tests + i;
    t->run();
  }
  return 0;  
}
