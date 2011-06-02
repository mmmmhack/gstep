#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "gdb.h"

/*
static void snooze() {
		struct timespec ts_in = {0, 1e6};
		struct timespec ts_out;
		nanosleep(&ts_in, &ts_out);
}
*/

int init() {
  int rc = gdb_init();
	return rc;
}// init


