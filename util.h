#ifndef _util_h
#define _util_h
#include <time.h>

// logging
#define LOG_LEVEL_TRACE 10 
#define LOG_LEVEL_DEBUG 20 
#define LOG_LEVEL_INFO  30
#define LOG_LEVEL_WARN  40
#define LOG_LEVEL_ERROR 50
#define LOG_LEVEL_ALWAYS 60
#define TRACE(s) do {if(log_level() <= LOG_LEVEL_TRACE) log_msg(LOG_LEVEL_TRACE, s, __FILE__, __LINE__);}while(0);
#define DEBUG(s) do {if(log_level() <= LOG_LEVEL_DEBUG) log_msg(LOG_LEVEL_DEBUG, s, __FILE__, __LINE__);}while(0);
#define INFO(s) do {if(log_level() <= LOG_LEVEL_INFO) log_msg(LOG_LEVEL_INFO, s, __FILE__, __LINE__);}while(0);
#define WARN(s) do {if(log_level() <= LOG_LEVEL_WARN) log_msg(LOG_LEVEL_WARN, s, __FILE__, __LINE__);}while(0);
#define ERROR(s) do {if(log_level() <= LOG_LEVEL_ERROR) log_msg(LOG_LEVEL_ERROR, s, __FILE__, __LINE__);}while(0);
#define LOG(s) log_msg(LOG_LEVEL_ALWAYS, s, __FILE__, __LINE__)
void log_msg(int level, const char* msg, const char* file, const int line);
int log_level();
void log_set_level(int level);

// error funcs
void sys_err(const char* msg);

// time funcs
//typedef struct timeval* timevalp;
struct timeval time_now();
struct timeval time_sub(const struct timeval* x, const struct timeval* y);
int time_greater(const struct timeval* x, const struct timeval* y);
void time_sleep(const struct timeval* t);
#endif
