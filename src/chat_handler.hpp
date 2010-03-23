#ifndef __CHAT_HANDLER_GUARD__
#define __CHAT_HANDLER_GUARD__

#include "evx/buffered_connection.hpp"
#include "connection_finder.hpp"

class chat_handler : public evx::buffered_handler_base
{
private:
	evx::buffered_connection::weak_ptr other;
	connection_type_t connection_type;
	bool successful; // true if the connection has successfuly managed to read some data and is now bound to a proper other end.
	
	void end_other_end(evx::buffered_connection &con);
	
public:
	chat_handler(evx::buffered_connection::weak_ptr c_other, connection_type_t c_connection_type)
	 : other(c_other), connection_type(c_connection_type)
	{
		successful = (connection_type == type_bridge)?false:true; // clients are only morphed when they have another endpoint.
	}
	
	void registered(evx::buffered_connection &con);
	void timeout(evx::buffered_connection &con);
	void data_readable(evx::buffered_connection &con);
	void socket_shutdown(evx::buffered_connection &con);
	void socket_close(evx::buffered_connection &con, int err);
};
#endif