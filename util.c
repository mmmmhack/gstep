#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

void sys_err(const char* msg) {
  perror(msg);
  exit(1);
}

void log_msg(const char* s, const char* file, const int line) {
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
  fprintf(fs, "[%s] %s:%d: %s\n", timestamp, file, line, s);
  fclose(fs);
}

// time funcs
struct timeval time_now() {
  struct timeval t = {0, 0};
  gettimeofday(&t, NULL);
  return t;
}

// returns diff = x - y
struct timeval time_sub(struct timeval* x, struct timeval* y) {
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
}

int time_greater(struct timeval* x, struct timeval* y) {
  if(x->tv_sec > y->tv_sec)
    return 1;
  else if(y->tv_sec > x->tv_sec)
    return 0;
  return x->tv_usec > y->tv_usec;
}

void time_sleep(struct timeval* t) {
  struct timespec ts = {t->tv_sec, t->tv_usec * 1000};
  nanosleep(&ts, NULL);
}

