#include "util.h"
#include "gdb.h"
#include "init.h"
#include "cleanup.h"

int run() {
  // run stepper
	// communicate
	gdb_read();
	gdb_write("help\n");
	gdb_read();
  return 0;
}

int main(int argc, char** argv) {
  int rc = 0;

  // process args
  // start gdb
  rc |= init();

  rc |= run();

  // cleanup
  rc |= cleanup();
  
  return rc;
}
