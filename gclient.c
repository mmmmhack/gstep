#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "util.h"

static int port_num = 6667;
static const char* addr_ip = "127.0.0.1";

const char* PROG_VERSION = "0.1 " __DATE__;

const char* USAGE = \
"gclient - gdb client program\n" \
"usage: \n"\
"  gclient [options] command_string\n"\
"\n" \
"Options:\n" \
"  -h    List all options, with brief explanations.\n" \
"  -v    Report program version and exit.\n"\
"\n" \
"Commands:\n" \
"  start_gdb\n"\
"    Starts a gdb child process, terminating any running gdb process first.\n"\
"\n"\
"  gdb_cmd COMMAND_STRING\n"\
"    Sends COMMAND_STRING to the gdb child process and reads a response.\n"\
"\n"\
"  quit_gdb\n"\
"    Terminates any running gdb process\n"\
"\n"\
"  quit_gserv\n"\
"    Exists the gserver program, terminating any running gdb process first.\n"\
"\n"\
"  debug_no_respond\n"\
"    Receives the command from the gclient process but doesn't do any\n"\
"    subsequent action.\n"\
;

static void usage() {
  printf(USAGE);
}

static void parse_options(int argc, char** argv) {
  while(--argc) {
    const char* arg = argv[argc];
    if(arg[0] != '-')
      continue;
    switch(arg[1]) {
    case 'h':
      usage();
      exit(0);
    case 'v':
      printf("gclient version %s\n", PROG_VERSION);
      exit(0);
    default:
      fprintf(stderr, "invalid option %s\n", arg);
      exit(1);
    }
  }
}


int main(int argc, char** argv) {
  log_set_level(LOG_LEVEL_TRACE);
  TRACE("beg main()");
  INFO(bdata(bformat("gclient started %s, pid: %d", ctime_now(), getpid())));

  // parse options
  parse_options(argc, argv);

  if(argc < 2) {
    fprintf(stderr, "missing request arg\n");
    exit(1);
  }

  int rc;

  // create socket
  TRACE("bef socket");
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(s < 0)
    sys_err("socket() failed");

  // connect
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
#ifdef OS_DARWIN
  sa.sin_len = sizeof(sa);
#endif
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_num);
  rc = inet_aton(addr_ip, &sa.sin_addr);
  if(rc == 0)
    sys_err("inet_aton() failed");
  TRACE("bef connect");
  rc = connect(s, (struct sockaddr*) &sa, sizeof(sa));
  if(rc < 0)
    sys_err("connect() failed");

  // send
  bstring request = bfromcstr(argv[1]);
	int i;
	for(i = 2; i < argc; ++i) {
		bcatcstr(request, " ");
		bcatcstr(request, argv[i]);
	}
  bcatcstr(request, "\n");   // requests must be terminated by a newline
  const char* msg = bdata(request);
  size_t msg_len = blength(request);
  TRACE(bdata(bformat("bef send: msg: [%s]", msg)));
  rc = send(s, msg, msg_len, 0);
  TRACE("aft send");
  if(rc < 0)
    sys_err("send() failed");
  if(rc != (int) msg_len)
    sys_err("send() incomplete");

  // receive
//  printf("--- recvd from server:\n");
  char buf[BUFSIZ];
  while(1) {
    TRACE("bef recv");
    rc = recv(s, buf, sizeof(buf), 0);
    TRACE("aft recv");

    // error
    if(rc < 0)
      sys_err("recv() failed");
    buf[rc] = '\0';

    // closed
    if(rc == 0) {
      DEBUG("recv() returned 0, quitting recv loop"); 
      break;
    }
    DEBUG(bdata(bformat("recv returned %d bytes: [%s]", rc, buf)));

		// strip '200 OK\n' from buf (only need to report errors)
		char* out = buf;
		if(!strncmp(out, "200 OK\n", 7))
			out += 7;

    // output 
    fputs(out, stdout);
  } 
  // add a final newline for cleaner output on cmd-line
  fputs("\n", stdout);

  // close socket
  TRACE("bef close()");
  rc = close(s);
  TRACE("aft close()");
  if(rc < 0)
    sys_err("close socket failed");
  
  TRACE("end main()");
  return 0;
}
