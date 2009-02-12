#ifndef __LISTEN_HANDLER_GUARD__
#define __LISTEN_HANDLER_GUARD__

#include <map>

#include "evx/buffered_connection.hpp"

#include "connection_finder.hpp"

class listen_handler
{
private:
	int listen_socket;
	struct ev_loop *loop;
	evx_io *watcher;
	
	connection_pool::ptr connections;
	std::string host_key_secret;
	
public:
	listen_handler(int c_listen_socket, const std::string &c_secret)
	 : listen_socket(c_listen_socket), connections(new connection_pool), host_key_secret(c_secret)
	{}
	
	void operator()(struct ev_loop *loop, evx_io *watcher, int revents);
};
#endif