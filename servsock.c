#include <unistd.h>
#include "servsock.h"

static int _listen_socket = -1;
static int _accept_socket = -1;

void servsock_set_listen_socket(int socket) {
  _listen_socket = socket;
}

// closes socket descriptor for forked processes
int servsock_close_listen_socket() {
  int rc = close(_listen_socket);
  return rc;
}

void servsock_set_accept_socket(int socket) {
  _accept_socket = socket;
}

// closes socket descriptor for forked processes
int servsock_close_accept_socket() {
  int rc = close(_accept_socket);
  return rc;
}


