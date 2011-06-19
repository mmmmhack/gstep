#ifndef _gdb_h
#define _gdb_h
#include <signal.h>

pid_t gdb_pid();
int gdb_start();
int gdb_read();
int gdb_write(const char* s);
int gdb_kill();
#endif
