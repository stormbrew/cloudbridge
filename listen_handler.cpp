#include <sys/socket.h>
#include <netinet/in.h>

#include "listen_handler.hpp"
#include "connection_finder.hpp"

using namespace evx;

void listen_handler::operator()(struct ev_loop *loop, evx_io *watcher, int revents)
{
	struct sockaddr_in remote_addr;
	socklen_t addr_len = sizeof(remote_addr);
	int socket = accept(listen_socket, reinterpret_cast<struct sockaddr*>(&remote_addr), &addr_len);
	
	buffered_connection::create_connection(loop, socket, buffered_connection::client_handler_ptr(new connection_finder(waiting_connections)));
}
