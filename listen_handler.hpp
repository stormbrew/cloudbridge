#ifndef __LISTEN_HANDLER_GUARD__
#define __LISTEN_HANDLER_GUARD__

#include <map>

#include "evx/buffered_connection.hpp"

class listen_handler
{
public:
	typedef std::map<std::string, evx::buffered_connection::weak_ptr> connection_map;
	
private:
	int listen_socket;
	struct ev_loop *loop;
	evx_io *watcher;
	
	std::tr1::shared_ptr<connection_map> waiting_connections;
	
public:
	listen_handler(int c_listen_socket)
	 : listen_socket(c_listen_socket), waiting_connections(new connection_map)
	{}
	
	void operator()(struct ev_loop *loop, evx_io *watcher, int revents);
};
#endif