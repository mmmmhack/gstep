#ifndef _util_h
#define _util_h
#include "time.h"

// error funcs
void sys_err(const char* msg);

// time funcs
//typedef struct timeval* timevalp;
struct timeval time_now();
struct timeval time_sub(struct timeval* a, struct timeval* b);
int time_greater(struct timeval* a, struct timeval* b);
void time_sleep(struct timeval* t);
#endif
