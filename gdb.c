#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
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
static const char* gdb_term = "(gdb) ";
static const struct timeval child_startup_wait_time = {0, 200000}; // 200 msec
static const struct timeval child_shutdown_wait_time = {0, 800000}; // 200 msec
static const struct timeval wait_cycle_time = {0, 10}; // 10 usec
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

// returns 0 if child is still running after wait time, else returns 1 if child exited
static int wait_for_child_startup(pid_t child_pid) {
  int rc = 0; // assume child success at start

  TRACE("beg wait_for_child_startup()");
  struct timeval beg_time = time_now();
  while(1) {
    // get child status
    if(!is_child_running(child_pid)) {
      ERROR("gdb child stopped running");
      rc = 1;
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
    time_sleep(&wait_cycle_time);
  }
  TRACE("end wait_for_child_startup()");
  return rc;
}

// returns 0 if child is stopped running before wait timeout, else returns 1 if child still running
static int wait_for_child_shutdown(pid_t child_pid) {
  int rc = 0; // assume success at start

  TRACE("beg wait_for_child_shutdown()");
  struct timeval beg_time = time_now();
  while(1) {
    // get child status
    if(!is_child_running(child_pid)) {
      INFO("gdb child stopped running");
      break;
    }
    // check for wait end
    struct timeval cur_time = time_now();
    struct timeval diff_time = time_sub(&cur_time, &beg_time);
    TRACE(bdata(bformat("diff_time: %d, %ld", diff_time.tv_sec, diff_time.tv_usec)));

    if(time_greater(&diff_time, &child_shutdown_wait_time)) {
      ERROR("max wait time reached");
      rc = 1;
      break;
    }
    // sleep
    time_sleep(&wait_cycle_time);
  }
  TRACE("end wait_for_child_shutdown()");
  return rc;
}

/* sets up pipes for gdb stdin,stdout, starts gdb
*/
int gdb_start() {
  TRACE("beg gdb_start()");

	// create pipe0
	int rc = pipe(pipe0);
	if(rc < 0) {
		ERROR("pipe0 failed");
    return -1;
  }
	// create pipe1
	rc = pipe(pipe1);
	if(rc < 0) { 
		ERROR("pipe1 failed");
    return -1;
  }

  DEBUG("after create pipes");

	// fork
  TRACE("bef fork");
	int pid = fork();
  TRACE("aft fork");
	if(pid < 0) {
		ERROR("fork failed");
    return -1;
  }
	// child
	if(pid == 0) {
    TRACE("bef stdio pipe switch");
		// switch stdout to pipe0
		int rc  = dup2(pipe0[1], STDOUT_FILENO);
		if(rc < 0) {
			ERROR("dup2 stdout failed");
      return -1;
    }
		// switch stdin to pipe1
		rc  = dup2(pipe1[0], STDIN_FILENO);
		if(rc < 0) {
			ERROR("dup2 stdin failed");
      return -1;
    }
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
			ERROR("execv failed");
      return -1;
    }
	}
  // parent
  else {
    INFO(bdata(bformat("aft fork: child pid: %d", pid)));

    // wait for min time needed to determine whether or not child process failed and died
    rc = wait_for_child_startup(pid);
    if(rc != 0) {
      gdb_set_pid(-1);
      ERROR("child startup failed");
      return -1;
    }
    gdb_set_pid(pid);
  }

  TRACE("end gdb_start()");
  return 0;
}// init

int has_terminator(const char* buf) {
  TRACE("beg has_terminator()");
  int blen = strlen(buf);
  int tlen = strlen(gdb_term);
  int i = blen - tlen;
  if(i < 0) {
    TRACE(bdata(bformat("buf len %d is less than terminator len %d, skipping comparison", blen, tlen)));
    return 0;
  }
  int rc = strcmp(buf + i, gdb_term);
  TRACE(bdata(bformat("strcmp returned: %d, buf+i: [%s], gdb_term: [%s]", rc, buf+i, gdb_term)));
  TRACE("end has_terminator()");
  return rc == 0;
}

/* Reads from gdb stdin until the gdb terminator is read. 
	 The terminator is '^(gdb)$\n'
*/
int gdb_read(bstring* bret) {
  TRACE("beg gdb_read");
  int rc = 0;
  char buf[BUFSIZ];
	int fd = pipe0[0];
	ssize_t nread;
  struct timeval short_sleep = {0, 1}; // 1 us sleep time

	while(1) {
    TRACE("bef socket read");
		nread = read(fd, buf, BUFSIZ);
    TRACE(bdata(bformat("aft socket read: nread: %d", nread)));
		if(nread < 0) {
			ERROR("read failed");
      rc = 1;
      break;
    }
		buf[nread] = 0;
		bcatcstr(*bret, buf);
	  DEBUG(bdata(bformat("read buf: [%s]", (*bret)->data)));
		if(has_terminator(buf)) {
			DEBUG("buf terminator found, quitting read loop");
			break;
		}
	  time_sleep(&short_sleep);	
	}
  TRACE("end gdb_read");
  return rc;
}

int gdb_write(const char* s) {
  TRACE("beg gdb_write()");
  int rc = 0;
	int fd = pipe1[1];
	ssize_t len = (ssize_t) strlen(s);
	ssize_t nwrite;
  TRACE(bdata(bformat("len: %d", len)));
  TRACE("bef write");
	nwrite = write(fd, s, len);
  TRACE("aft write");
  TRACE(bdata(bformat("nwrite: %d", nwrite)));
	if(nwrite != len) {
    rc = -1;
		ERROR("write failed");
  }
  return rc;
  TRACE("end gdb_write()");
}

int gdb_kill() {
  if(_gdb_pid == -1) {
    ERROR("no current gdb process");
    return -1;
  }

  // kill child gdb
  int rc = kill(_gdb_pid, SIGTERM);
  if(rc)
    ERROR(bdata(bformat("kill() gdb pid %d failed: rc: %d", _gdb_pid, rc)));
  
  // dezombify
  rc = wait_for_child_shutdown(_gdb_pid);

  _gdb_pid = -1;
  return rc;
}

