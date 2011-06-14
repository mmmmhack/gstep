#include <sys/types.h>
#include <sys/time.h>
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
static const char* exe = "gdb";
static char* gdb_args[] = {"gdb", NULL};
static int _gdb_pid = -1;
static const char* gdb_term = "\n(gdb) ";
static const struct timeval child_startup_wait_time = {0, 200000}; // 200 msec
pid_t gdb_pid() {
  return _gdb_pid;
}
static void gdb_set_pid(pid_t pid) {
  _gdb_pid = pid;
}

// returns 1 if child is still running
static int is_child_running(pid_t child_pid) {
  int status = 0;
  TRACE("bef waitpid");
  pid_t pid = waitpid(child_pid, &status, WNOHANG);
  TRACE(bdata(bformat("aft waitpid: pid: %d, status: %d", pid, status)));
  return pid == 0;
}

// returns 1 if child is still running after wait time, else returns 0 if child exited
static int wait_for_child_startup(pid_t child_pid) {
  int rc = 1; // assume child success at start

  TRACE("beg wait_for_child_startup()");
  struct timeval beg_time = time_now();
  struct timeval cycle_sleep_time = {0, 1};  // 1us sleep time
  while(1) {
    // get child status
    if(!is_child_running(child_pid)) {
      ERROR("gdb child stopped running");
      rc = 0;
      break;
    }
    // check for wait end
    struct timeval cur_time = time_now();
    struct timeval diff_time = time_sub(&cur_time, &beg_time);
    TRACE(bdata(bformat("diff_time: %d, %ld", diff_time.tv_sec, diff_time.tv_usec)));

    if(time_greater(&diff_time, &child_startup_wait_time)) {
      INFO("max wait time reached");
      break;
    }
    // sleep
    time_sleep(&cycle_sleep_time);
  }
  TRACE("end wait_for_child_startup()");
  return rc;
}

/* sets up pipes for gdb stdin,stdout, starts gdb
*/
int gdb_start() {
  TRACE("beg gdb_start()");

	// create pipe0
	int rc = pipe(pipe0);
	if(rc < 0) 
		sys_err("pipe0 failed");
	// create pipe1
	rc = pipe(pipe1);
	if(rc < 0) 
		sys_err("pipe1 failed");

  DEBUG("after create pipes");

	// fork
  TRACE("bef fork");
	int pid = fork();
  TRACE("aft fork");
	if(pid < 0)
		sys_err("fork failed");
	// child
	if(pid == 0) {
    TRACE("bef stdio pipe switch");
		// switch stdout to pipe0
		int rc  = dup2(pipe0[1], STDOUT_FILENO);
		if(rc < 0)
			sys_err("dup2 stdout failed");
		// switch stdin to pipe1
		rc  = dup2(pipe1[0], STDIN_FILENO);
		if(rc < 0)
			sys_err("dup2 stdin failed");
    TRACE("aft stdio pipe switch");

		// switch to target prog
    bstring gdb_args_str = bfromcstr("");
    char** arg = gdb_args;
    while(*arg) {
      if(blength(gdb_args_str))
        bcatcstr(gdb_args_str, ", ");
      bcatcstr(gdb_args_str, *arg++); 
    }
    DEBUG(bdata(bformat("bef execvp(): exe: [%s], gdb_args: [%s]", exe, gdb_args_str->data)));
		rc = execvp(exe, gdb_args);
    DEBUG(bdata(bformat("aft execvp(): rc: %d", rc)));
		if(rc < 0) {
			sys_err("execv failed");
    }
	}
  // parent
  else {
    INFO(bdata(bformat("aft fork: child pid: %d", pid)));

    // wait for min time needed to determine whether or not child process failed and died
    rc = wait_for_child_startup(pid);
    if(rc)
      gdb_set_pid(pid);
    else
      gdb_set_pid(-1);
  }

  if(gdb_pid() == -1)
    rc = 1;
  else
    rc = 0;

  TRACE("end gdb_start()");
  return rc;
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
			DEBUG(bdata(bformat("--- read_child: %d bytes\n%s\n", rdata->slen, rdata->data)));
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

void gdb_kill() {
  if(_gdb_pid == -1) {
    WARN("no current gdb process");
    return;
  }
  // TODO: add code to kill child gdb

}
