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

int main(int argc, char** argv) {
  if(argc < 2) {
    fprintf(stderr, "missing request arg\n");
    exit(1);
  }

  int rc;

  // create socket
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(s < 0)
    sys_err("socket() failed");

  // connect
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_len = sizeof(sa);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_num);
  rc = inet_aton(addr_ip, &sa.sin_addr);
  if(rc == 0)
    sys_err("inet_aton() failed");
  rc = connect(s, (struct sockaddr*) &sa, sizeof(sa));
  if(rc < 0)
    sys_err("connect() failed");

  // send
//  char* msg = "howdy partner\n";
//  const char* request = argv[1];
  bstring request = bfromcstr(argv[1]);
  bcatcstr(request, "\n");   // requests must be terminated by a newline
//  const char* msg = request;
//  size_t msg_len = strlen(msg);
  const char* msg = bdata(request);
  size_t msg_len = blength(request);
  LOG("bef send");
  rc = send(s, msg, msg_len, 0);
  LOG("aft send");
  if(rc < 0)
    sys_err("send() failed");
  if(rc != (int) msg_len)
    sys_err("send() incomplete");

  // receive
  printf("--- recvd from server:\n");
  char buf[BUFSIZ];
  while(1) {
    rc = recv(s, buf, sizeof(buf), 0);
    // error
    if(rc < 0)
      sys_err("recv() failed");
    buf[rc] = '\0';

    // closed
    if(rc == 0)
      break;

    // output 
    fputs(buf, stdout);

    // msg ends at newline
    if(buf[rc-1] == '\n')
      break;
  } 

  // close socket
  LOG("bef close()");
  rc = close(s);
  LOG("aft close()");
  if(rc < 0)
    sys_err("close socket failed");
  
  return 0;
}
