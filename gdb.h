#ifndef _gdb_h
#define _gdb_h
#include <signal.h>

pid_t gdb_pid();
int gdb_start();
void gdb_read();
void gdb_write(const char* s);
void gdb_kill();
#endif
