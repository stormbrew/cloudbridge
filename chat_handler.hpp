#ifndef __CHAT_HANDLER_GUARD__
#define __CHAT_HANDLER_GUARD__

#include "evx/buffered_connection.hpp"

class chat_handler : public evx::buffered_handler_base
{
private:
	evx::buffered_connection::weak_ptr other;
	
public:
	chat_handler(evx::buffered_connection::weak_ptr c_other)
	 : other(c_other)
	{}
	
	void registered(evx::buffered_connection &con);
	void timeout(evx::buffered_connection &con);
	void data_readable(evx::buffered_connection &con);
	void socket_shutdown(evx::buffered_connection &con);
	void socket_close(evx::buffered_connection &con, int err);
};
#endif