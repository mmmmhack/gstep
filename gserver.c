// gserv.c  : gdb server

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // mac include for sockaddr_in
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "util.h"
#include "gdb.h"

static in_port_t port_num = 6667;
static const int MAX_PENDING = 0;
static int _quit = 0;

typedef bstring (*CmdFunc)(bstring request);

struct Cmd {
  const char* name;
  CmdFunc handler;
};

bstring start_gdb(bstring req) {
//  return bfromcstr("501 Not Implemented\n");
  int rc = gdb_start();
  if(rc) {
    return bfromcstr("502 Gdb Failed To Start\n");
  }
  return bfromcstr("200 OK\n");
}

bstring quit_gdb(bstring req) {
  return bfromcstr("501 Not Implemented\n");
}

void kill_gdb() {
  gdb_kill();  
}

bstring quit_gserv(bstring req) {
  kill_gdb();
  INFO("quitting");
  _quit = 1;
  return bfromcstr("200 OK\n");
}

static struct Cmd cmds[] = {
  {"start_gdb", start_gdb, },
  {"quit_gdb", quit_gdb, },
  {"quit_gserv", quit_gserv, },
};
static const int num_cmds = sizeof(cmds) / sizeof(cmds[0]);

// read until a newline is recvd
static bstring read_msg(int s) {
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
  TRACE("beg serve_request");
  bstring response = bfromcstr("200 OK\n");

  // read request
  TRACE("bef read_msg");
  bstring request = read_msg(sock);
  TRACE(bdata(bformat("aft read_msg: data:\n%s", request->data)));

  // find request handler
  TRACE("bef find_cmd");
  struct Cmd* cmd = find_cmd(request);
  if(cmd == NULL) 
    bassigncstr(response, "400 Bad Request\n");
  DEBUG(bdata(bformat("found command: [%s]", cmd->name)));

  // call handler
  response = cmd->handler(request);

  // send response
  TRACE("bef send");
  int rc = send(sock, response->data, response->slen, 0);
  TRACE("aft send");
  if(rc < 0)
    sys_err("send() failed");
  
  bdestroy(request);
  bdestroy(response);
  TRACE("end serve_request");
}

int main(int argc, char** argv) {
  log_set_level(LOG_LEVEL_TRACE);
  int rc;
  INFO(bdata(bformat("beg main(): pid: %d", getpid())));
  
  // just fire up a socket to listen and wait for commands

  // create
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
  if(s == -1) 
    sys_err("socket failed");
  INFO("socket created");

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

  while(!_quit) {
    // accept
    struct sockaddr_in ca;
    memset(&ca, 0, sizeof(ca));
    socklen_t ca_len = sizeof(ca);
    int t = accept(s, (struct sockaddr*) &ca, &ca_len);
    if(t < 0)
      sys_err("accept failed");
    INFO("accepted");

    // handle request
    serve_request(t);
  }

  // close
  rc = close(s);
  if(rc == -1)
    sys_err("close failed");
  INFO("socket closed");

  INFO("end main()");
  return 0;
}
