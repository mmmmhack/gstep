#ifndef _gdb_h
#define _gdb_h
#include <signal.h>

pid_t gdb_pid();
//pid_t gdb_set_pid(pid_t pid);
int gdb_init();
void gdb_read();
void gdb_write(const char* s);
#endif
