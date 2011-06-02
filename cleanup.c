#include <sys/wait.h>
#include <signal.h>
#include "util.h"
#include "gdb.h"
#include "cleanup.h"

//static int wait_gone_timeout = 1; // secs
static struct timeval wait_timeout = {1, 0}; // 1 second

// returns 1 if child exited, else 0
static int wait_for_child_gone() {
  struct timeval beg_time = time_now();
  struct timeval sleep_time = {0, 1000};
  int status;
  int timeout = 0;
  while(!timeout) {
    pid_t pid = gdb_pid();
    pid_t wpid = waitpid(pid, &status, WNOHANG);
    if(wpid == pid) // child done
      break;

    // wait a little
    time_sleep(&sleep_time);
    struct timeval now_time = time_now();
    struct timeval wait_time = time_sub(&now_time, &beg_time);
    if(time_greater(&wait_time, &wait_timeout))
      timeout = 1;
  }
  if(timeout)
    return 0;
  return 1;
}

static int term_child() {
  pid_t pid = gdb_pid();
  int rc = kill(pid, SIGTERM);  
  return rc;
}

int cleanup() {
	// shutdown child
	gdb_write("quit\n");
  int rc = wait_for_child_gone();
  if(!rc)
    rc = term_child();
  return rc;
}

