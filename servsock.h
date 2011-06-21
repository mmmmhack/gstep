#ifndef servsock_h
#define servsock_h
void servsock_set_listen_socket();
int servsock_close_listen_socket();
void servsock_set_accept_socket();
int servsock_close_accept_socket();
#endif
