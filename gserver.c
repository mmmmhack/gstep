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

static in_port_t port_num = 6667;
static const int MAX_PENDING = 0;
static int _quit = 0;

typedef bstring (*CmdFunc)(bstring request);

struct Cmd {
  const char* name;
  CmdFunc handler;
};

bstring start_gdb(bstring req) {
  return bfromcstr("501 Not Implemented\n");
}

bstring quit_gdb(bstring req) {
  return bfromcstr("501 Not Implemented\n");
}

void kill_gdb() {
  
}

bstring quit_gserv(bstring req) {
  kill_gdb();
  LOG("quitting\n");
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
    LOG("bef recv");
    int rc = recv(s, buf, BUFSIZ, 0);
    LOG("aft recv");
    // error
    if(rc < 0)
      sys_err("recv failed");

    // append data
    buf[rc] = '\0';
    bcatcstr(msg, buf);
    LOG(bdata(bformat("buf: [%s]\n", buf)));

    // if nothing, premature end-of-message
    if(rc == 0)
      sys_err("recv ended before newline");

    // if newline, we're done
    if(buf[rc-1] == '\n')
      break;
  }
  LOG("aft while");
  return msg;
}

static struct Cmd* find_cmd(bstring request) {
  int i;
  for(i = 0; i < num_cmds; ++i) {
    struct Cmd* cmd = cmds + i;
//    if(strncmp(cmd->name, bdata(request), strlen(cmd->name))==0)
    char* pc = bdata(request);
    if(strncmp(cmd->name, pc, strlen(cmd->name))==0)
      return cmd;
  }
  LOG(bdata(bformat("command not found for request: [%s]", bdata(request))));
  return NULL;
}

static void serve_request(int sock) {
  LOG("beg serve_request");
  bstring response = bfromcstr("200 OK\n");

  // read request
  LOG("bef read_msg");
  bstring request = read_msg(sock);
  LOG(bdata(bformat("aft read_msg: data:\n%s", request->data)));

  // find request handler
  LOG("bef find_cmd");
  struct Cmd* cmd = find_cmd(request);
  if(cmd == NULL) 
    bassigncstr(response, "400 Bad Request\n");

  // call handler
  response = cmd->handler(request);

  // send response
  LOG("bef send");
  int rc = send(sock, response->data, response->slen, 0);
  LOG("aft send");
  if(rc < 0)
    sys_err("send() failed");
  
  bdestroy(request);
  bdestroy(response);
  LOG("end serve_request");
}

int main(int argc, char** argv) {
  int rc;

  // just fire up a socket to listen and wait for commands

  // create
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
  if(s == -1) 
    sys_err("socket failed");
  printf("socket created\n");

  // bind
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET; 
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port_num);
  rc = bind(s, (struct sockaddr*)&sa, sizeof(sa));
  if(rc < 0)
    sys_err("bind failed");
  printf("socket bound\n");

  // listen
  rc = listen(s, MAX_PENDING);
  if(rc < 0)
    sys_err("listen failed");
  printf("listening\n");

  while(!_quit) {
    // accept
    struct sockaddr_in ca;
    memset(&ca, 0, sizeof(ca));
    socklen_t ca_len = sizeof(ca);
    int t = accept(s, (struct sockaddr*) &ca, &ca_len);
    if(t < 0)
      sys_err("accept failed");
    printf("accepted\n");

    // echo
//    echo(t);
    // handle request
    serve_request(t);
  }

  // close
  rc = close(s);
  if(rc == -1)
    sys_err("close failed");
  printf("socket closed\n");
  return 0;

}
