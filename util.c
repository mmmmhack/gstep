#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

static const char* log_level_desc(int level) {
  switch(level) {
  case LOG_LEVEL_TRACE: return "TRACE";
  case LOG_LEVEL_DEBUG: return "DEBUG";
  case LOG_LEVEL_INFO: return "INFO";
  case LOG_LEVEL_WARN: return "WARN";
  case LOG_LEVEL_ERROR: return "ERROR";
  case LOG_LEVEL_ALWAYS: return "LOG";
  default: return "UNKNOWN";
  }
}

static int _log_level = LOG_LEVEL_INFO;

int log_level() {return _log_level;}
void log_set_level(int level) {_log_level = level;}

void sys_err(const char* msg) {
  ERROR(msg);
  perror(msg);
  exit(1);
}

void log_msg(int level, const char* s, const char* file, const int line) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char timestamp[128];
  sprintf(timestamp, "%ld.%06d", tv.tv_sec, tv.tv_usec);

  pid_t pid = getpid();  
  char fname[128];
  sprintf(fname, "log/%d.log", pid);
  FILE* fs = fopen(fname, "a");
  if(!fs)
    sys_err("fopen() failed");
  // add errno detail if any for ERROR logging
  char* err_pre = "";
  char* err_detail = "";
  if(level >= LOG_LEVEL_ERROR && errno) {
    err_pre = " error detail: ";
    err_detail = strerror(errno);
  }
  fprintf(fs, "[%s] %s: %s:%d: %s%s%s\n", timestamp, log_level_desc(level), file, line, s, err_pre, err_detail);
  fflush(fs);
  fclose(fs);
}

// time funcs
struct timeval time_now() {
  struct timeval t = {0, 0};
  gettimeofday(&t, NULL);
  return t;
}

// returns diff = x - y
struct timeval time_sub(const struct timeval* xp, const struct timeval* yp) {
  struct timeval xl, yl;
  memcpy(&xl, xp, sizeof(xl));
  memcpy(&yl, yp, sizeof(yl));
  struct timeval* x = &xl;
  struct timeval* y = &yl;

/* algorithm below is from libc docs, but doesn't work in my opinion
  // Perform the carry for the later subtraction by updating b. 
  if(x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if(x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  // Compute the time remaining to wait. `tv_usec' is certainly positive.
  struct timeval result;
  result.tv_sec = x->tv_sec - y->tv_sec;
  result.tv_usec = x->tv_usec - y->tv_usec;
  return result;
*/
  // subtract lesser from greater
  int neg_result = 0;
  if(time_greater(y, x)) {
    neg_result = 1;
    x = &yl;
    y = &xl;
  }
  struct timeval result;
  // subtract usec, borrow if needed
  if(x->tv_usec < y->tv_usec) {
    x->tv_sec -= 1;
    x->tv_usec += 1e6;
  }
  result.tv_usec = x->tv_usec - y->tv_usec;
  result.tv_sec = x->tv_sec - y->tv_sec;
  if(neg_result) {
    result.tv_sec = -result.tv_sec;
    result.tv_usec = -result.tv_usec;
  }
  return result;
}

int time_greater(const struct timeval* x, const struct timeval* y) {
  if(x->tv_sec > y->tv_sec)
    return 1;
  else if(y->tv_sec > x->tv_sec)
    return 0;
  return x->tv_usec > y->tv_usec;
}

void time_sleep(const struct timeval* t) {
  struct timespec ts = {t->tv_sec, t->tv_usec * 1000};
  nanosleep(&ts, NULL);
}

