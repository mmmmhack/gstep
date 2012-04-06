// gserver.c  : gdb server

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // mac include for sockaddr_in
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "util.h"
#include "servsock.h"
#include "gdb.h"

// response codes
#define C200_OK "200 OK\n"
#define C400_BAD_REQ "400 Bad Request\n"
#define C501_NOT_IMPL "501 Not Implemented\n"
#define C502_GDB_FAIL_STOP "502 Gdb Failed To Stop\n"
#define C503_GDB_FAIL_START "503 Gdb Failed To Start\n"
#define C504_GDB_NOT_RUNNING "504 Gdb Not Running\n"
#define C505_FAILED_SEND_GDB_CMD "505 Failed Sending Gdb Command\n"
#define C506_FAILED_RECV_GDB_RESP "506 Failed Receiving Gdb Response\n"


const char* PROG_VERSION = "0.1 " __DATE__;

const char* USAGE = \
"gserver - gdb server program\n" \
"A tool for debugging with gdb in other programs such as vim\n" \
"usage: \n"\
"  gserver [options]\n"\
"\n" \
"Options:\n" \
"  -h         List all options, with brief explanations.\n" \
"  -v         Report program version and exit.\n" \
"Environment variables:\n" \
"  GSERVER_PORT	: port number to communicate with gclient (default=6667)\n" \
;

static in_port_t port_num = 6667;
static const int MAX_PENDING = 0;
static int _quit = 0;

typedef bstring (*CmdFunc)(bstring request);

struct Cmd {
  const char* name;
  CmdFunc handler;
};

static bstring start_gdb(bstring req);
static bstring gdb_cmd(bstring req);
static bstring quit_gdb(bstring req);
static bstring quit_gserv(bstring req);
static bstring help(bstring req);
static bstring debug_no_respond(bstring req);
// commands
static struct Cmd cmds[] = {
  {"start_gdb", start_gdb, },
  {"gdb_cmd", gdb_cmd, },
  {"quit_gdb", quit_gdb, },
  {"quit_gserv", quit_gserv, },
  {"help", help, },
  {"debug_no_respond", debug_no_respond, },
};
static const int num_cmds = sizeof(cmds) / sizeof(cmds[0]);

static void usage() {
  printf("%s", USAGE);
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
      printf("gserver version %s\n", PROG_VERSION);
      exit(0);
    default:
      fprintf(stderr, "invalid option %s\n", arg);
      exit(1);
    }
  }
}

// cmd-handler: creates the child gdb process
static bstring start_gdb(bstring req) {
  bstring bret = bfromcstr(C200_OK);
  int rc;

  // stop first if running
  if(gdb_pid() != -1) {
    rc = gdb_kill(); 
    if(rc)
      return bfromcstr(C502_GDB_FAIL_STOP); 
  }
  rc = gdb_start();
  if(rc)
    return bfromcstr(C503_GDB_FAIL_START);

	// read initial text from gdb
  bstring gdb_response = bfromcstr("");
  TRACE("BEF gdb_read");
  rc = gdb_read(&gdb_response);
  TRACE("AFT gdb_read");
  TRACE(bdata(bformat("rc: %d", rc)));
  if(rc)
    return bfromcstr(C506_FAILED_RECV_GDB_RESP);
  // concat gdb response to gserver response
  bconcat(bret, gdb_response);

  return bret;
}

// cmd-handler: sends a command to the child gdb process
static bstring gdb_cmd(bstring req) {
  TRACE("beg gdb_cmd()");
  TRACE(bdata(bformat("req: [%s]", req->data)));
  bstring bret = bfromcstr(C200_OK);
  int rc;

  // make sure gdb is running
  if(gdb_pid() == -1) {
    return bfromcstr(C504_GDB_NOT_RUNNING); 
  }
  // send cmd
  bstring cmd = bmidstr(req, 7, req->slen);
  TRACE("bef gdb_write");
  rc = gdb_write(bdata(cmd));
  TRACE(bdata(bformat("aft gdb_write: rc: %d", rc)));
  if(rc)
    return bfromcstr(C505_FAILED_SEND_GDB_CMD);
  // read response
  bstring gdb_response = bfromcstr("");
  TRACE("BEF gdb_read");
  rc = gdb_read(&gdb_response);
  TRACE("AFT gdb_read");
  TRACE(bdata(bformat("rc: %d", rc)));
  if(rc)
    return bfromcstr(C506_FAILED_RECV_GDB_RESP);
  // concat gdb response to gserver response
  bconcat(bret, gdb_response);

  TRACE("end gdb_cmd()");
  return bret;
}

// cmd-handler: terminates the child gdb process
static bstring quit_gdb(bstring req) {
  bstring bret = bfromcstr(C200_OK);
  int rc;
  if(gdb_pid() == -1)
    return bfromcstr(C504_GDB_NOT_RUNNING);
  rc = gdb_kill(); 
  if(rc)
    return bfromcstr(C502_GDB_FAIL_STOP); 
  return bret;
}

// cmd-handler: terminates the gserver program
static bstring quit_gserv(bstring req) {
  bstring bret = bfromcstr(C200_OK);
  // stop gdb if running
  if(gdb_pid() != -1) {
    int rc = gdb_kill();
    if(rc)
      bassigncstr(bret, C502_GDB_FAIL_STOP);
  }
  INFO("quitting");
  _quit = 1;
  return bret;
}

// cmd-handler: help - lists commands
static bstring help(bstring req) {
  bstring bret = bfromcstr(C200_OK);
	bstring bcommands = bfromcstr("commands: \n");
	int i;
  for(i = 0; i < num_cmds; ++i) {
    struct Cmd* cmd = cmds + i;
		char ln[256];
		sprintf(ln, "  %s\n", cmd->name);
		bconcat(bcommands, bfromcstr(ln));
	} 
  bconcat(bret, bcommands);
  return bret;
}

// cmd-handler: debug handler where the server never responds to the client
static bstring debug_no_respond(bstring req) {
  TRACE("beg debug_no_respond");
  TRACE("end debug_no_respond");
  return bfromcstr("do not respond");
}

// reads a request string from gclient: reads from socket until a newline is recvd
static bstring read_msg_from_client(int s) {
  TRACE("beg read_msg_from_client()");
  char buf[BUFSIZ];
  bstring msg = bfromcstr("");
  
  while(1) {
    TRACE("bef recv");
    int rc = recv(s, buf, BUFSIZ, 0);
    TRACE("aft recv");
    // error
    if(rc < 0)
      sys_err("recv failed");

    // append data
    buf[rc] = '\0';
    bcatcstr(msg, buf);
    TRACE(bdata(bformat("buf: [%s]", buf)));

    // if nothing, premature end-of-message
    if(rc == 0)
      sys_err("recv ended before newline");

    // if newline, we're done
    if(buf[rc-1] == '\n')
      break;
  }
  TRACE("aft while");
  TRACE("end read_msg_from_client()");
  return msg;
}

static struct Cmd* find_cmd(bstring request) {
  int i;
  for(i = 0; i < num_cmds; ++i) {
    struct Cmd* cmd = cmds + i;
    char* pc = bdata(request);
    if(strncmp(cmd->name, pc, strlen(cmd->name))==0)
      return cmd;
  }
  INFO(bdata(bformat("command not found for request: [%s]", bdata(request))));
  return NULL;
}

static void serve_request(int sock) {
  TRACE("beg serve_request()");
  bstring response = bfromcstr("200 OK\n");

  // read request
  TRACE("bef read_msg_from_client");
  bstring request = read_msg_from_client(sock);
  TRACE("aft read_msg_from_client");
  TRACE(bdata(bformat("client request: [%s]", request->data)));

  // find request handler
  TRACE("bef find_cmd");
  struct Cmd* cmd = find_cmd(request);
  TRACE("aft find_cmd");
  if(cmd == NULL) 
    bassigncstr(response, "400 Bad Request\n");
  else {
    DEBUG(bdata(bformat("found command: [%s]", cmd->name)));

    // call handler
    TRACE("bef call handler");
    response = cmd->handler(request);
    TRACE("aft call handler");
  }

  // special case: no response
  if(!strcmp((char*)request->data, "debug_no_respond")) {
    TRACE("not sending a response for this request");
    bdestroy(request);
    bdestroy(response);
    return;
  }

  // send response
  TRACE(bdata(bformat("sending %d bytes: [%s]", response->slen, response->data)));
  TRACE("bef send");
  int rc = send(sock, response->data, response->slen, 0);
  TRACE("aft send");
  TRACE(bdata(bformat("rc: %d", rc)));
  if(rc < 0)
    sys_err("send failed");
  
  // close socket
  TRACE("bef close accept socket");
  rc = close(sock);
  TRACE("aft close accept socket");
  TRACE(bdata(bformat("rc: %d", rc)));
  if(rc)
    sys_err("close socket failed");

  bdestroy(request);
  bdestroy(response);
  TRACE("end serve_request()");
}

int main(int argc, char** argv) {
  log_set_level(LOG_LEVEL_TRACE);
  TRACE("beg main");
  INFO(bdata(bformat("gserver started %s, pid: %d", ctime_now(), getpid())));
  
  int rc;

  // parse options
  parse_options(argc, argv);

	// get port from env
	char* port_str = getenv("GSERVER_PORT");
	if(port_str != NULL) {
		port_num = atoi(port_str);
	}

  // just fire up a socket to listen and wait for commands

  // create
  TRACE("bef socket");
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
  TRACE("aft socket");
  if(s == -1) 
    sys_err("socket failed");
  INFO(bdata(bformat("socket %d created", socket)));
  // save socket value for access by forked processes
  servsock_set_listen_socket(s);

  // bind
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET; 
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port_num);
  rc = bind(s, (struct sockaddr*)&sa, sizeof(sa));
  if(rc < 0)
    sys_err("bind failed");
  INFO("socket bound");

  // listen
  rc = listen(s, MAX_PENDING);
  if(rc < 0)
    sys_err("listen failed");
  INFO("listening");
	printf("listening on port %d\n", port_num);

  while(!_quit) {
    // accept
    struct sockaddr_in ca;
    memset(&ca, 0, sizeof(ca));
    socklen_t ca_len = sizeof(ca);
    TRACE("bef accept");
    int t = accept(s, (struct sockaddr*) &ca, &ca_len);
    TRACE("aft accept");
    if(t < 0)
      sys_err("accept failed");
    INFO("accepted");
    servsock_set_accept_socket(t);

    // handle request
    serve_request(t);
  }

  // close
  TRACE("bef close listen socket");
  rc = close(s);
  TRACE("aft close listen socket");
  if(rc == -1)
    sys_err("close failed");
  INFO("socket closed");

  TRACE("end main()");
  return 0;
}
