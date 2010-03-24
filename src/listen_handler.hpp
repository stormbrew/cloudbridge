#ifndef __LISTEN_HANDLER_GUARD__
#define __LISTEN_HANDLER_GUARD__

#include <map>

#include "evx/buffered_connection.hpp"

#include "connection_finder.hpp"
#include "connection_pool.hpp"

class listen_handler
{
private:
	int listen_socket;
	struct ev_loop *loop;
	evx_io *watcher;
	
	connection_pool::ptr connections;
	std::string host_key_secret;
	
public:
	listen_handler(int c_listen_socket, const std::string &c_secret, connection_pool::ptr c_connections)
	 : listen_socket(c_listen_socket), host_key_secret(c_secret),
	   connections(c_connections)
	{}
	
	void operator()(struct ev_loop *loop, evx_io *watcher, int revents);
};
#endif