#ifndef __CONNECTION_FINDER_GUARD__
#define __CONNECTION_FINDER_GUARD__

#include "evx/buffered_connection.hpp"

#include "listen_handler.hpp"

// this class is a state machine class that matches itself to another connection_finder class
// based on the first line of text passed in to socket. Two sockets with the same first line
// will be matched, and then their handlers will be replaced with the chat_handler class
// that passes messages back and forth between them.
class connection_finder : public evx::buffered_handler_base
{
private:
	std::tr1::shared_ptr<listen_handler::connection_map> pool;
	std::tr1::shared_ptr<std::string> mapped;
	
public:	
	connection_finder(std::tr1::shared_ptr<listen_handler::connection_map> c_pool)
	 : pool(c_pool)
	{}
	
	void morph(evx::buffered_connection &this_con, evx::buffered_connection::ptr other_con);
	
	void data_readable(evx::buffered_connection &con);
	void socket_shutdown(evx::buffered_connection &con);
	void socket_close(evx::buffered_connection &con, int err);
};
#endif