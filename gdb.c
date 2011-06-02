#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bstrlib.h"

#include "util.h"
#include "gdb.h"

static int pipe0[2];
static int pipe1[2];
static const char* exe = "/home/wknight/bin/gdb";
static char* gdb_args[] = {"gdb", NULL};
static int _gdb_pid = -1;
static const char* gdb_term = "\n(gdb) ";
	
pid_t gdb_pid() {
  return _gdb_pid;
}
static void gdb_set_pid(pid_t pid) {
  _gdb_pid = pid;
}

/* sets up pipes for gdb stdin,stdout, starts gdb
*/
int gdb_init() {
	// create pipe0
	int rc = pipe(pipe0);
	if(rc < 0) 
		sys_err("pipe0 failed");
	// create pipe1
	rc = pipe(pipe1);
	if(rc < 0) 
		sys_err("pipe1 failed");

	// fork
	int pid = fork();
	if(pid < 0)
		sys_err("fork failed");
	// child
	if(pid == 0) {
		// switch stdout to pipe0
		int rc  = dup2(pipe0[1], STDOUT_FILENO);
		if(rc < 0)
			sys_err("dup2 stdout failed");
		// switch stdin to pipe1
		rc  = dup2(pipe1[0], STDIN_FILENO);
		if(rc < 0)
			sys_err("dup2 stdin failed");

		// switch to target prog
		rc = execv(exe, gdb_args);
		if(rc < 0) 
			sys_err("execv failed");
	}
  // parent
  else
    gdb_set_pid(pid);

  return 0;
}// init

int has_terminator(const char* buf) {
  int blen = strlen(buf);
  int tlen = strlen(gdb_term);
  int i = blen - tlen;
  if(i < 0)
    return 0;
  int rc = strcmp(buf + i, gdb_term);
  return rc == 0;
}

/* Reads from gdb stdin until the gdb terminator is read. 
	 The terminator is '^(gdb)$\n'
*/
void gdb_read() {
  char buf[BUFSIZ];
	int fd = pipe0[0];
	ssize_t nread;
  struct timeval short_sleep = {0, 1}; // 1 us sleep time
  bstring rdata = bfromcstr("");

	while(1) {
		nread = read(fd, buf, BUFSIZ);
		if(nread < 0)
			sys_err("read failed");
		buf[nread] = 0;
		bcatcstr(rdata, buf);
		if(has_terminator(buf)) {
			printf("--- read_child: %d bytes\n%s\n", rdata->slen, rdata->data);
			break;
		}
	  time_sleep(&short_sleep);	
	}
  bdestroy(rdata);
}

void gdb_write(const char* s) {
	int fd = pipe1[1];
	ssize_t len = (ssize_t) strlen(s);
	ssize_t nwrite;
	nwrite = write(fd, s, len);
	if(nwrite != len) 
		sys_err("write failed");
}


