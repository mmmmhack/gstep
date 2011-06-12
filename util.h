#ifndef _util_h
#define _util_h
#include <time.h>

// logging
#define LOG(s) log_msg(s, __FILE__, __LINE__)
void log_msg(const char* s, const char* file, const int line);

// error funcs
void sys_err(const char* msg);

// time funcs
//typedef struct timeval* timevalp;
struct timeval time_now();
struct timeval time_sub(struct timeval* a, struct timeval* b);
int time_greater(struct timeval* a, struct timeval* b);
void time_sleep(struct timeval* t);
#endif
