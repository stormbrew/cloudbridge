#include "chat_handler.hpp"

using namespace evx;

void chat_handler::registered(evx::buffered_connection &con)
{}
void chat_handler::timeout(evx::buffered_connection &con)
{}

void chat_handler::data_readable(buffered_connection &con)
{
	// TODO: Implement ignoring of initial 100 Continue header.
	buffered_connection::ptr other_ptr = other.lock();

	if (!other_ptr || other_ptr->closed())
	{
		// the other end is gone, so let's start killing this connection.
		con.shutdown();
	} else {
		// dump all of our unread data to the other end.
		buffered_connection::iterator begin = con.read_begin(), end = con.read_end();
		other_ptr->write(begin, end);
		con.set_read_begin(end);			
	}
}
void chat_handler::socket_shutdown(buffered_connection &con)
{

	// shut down the other end if it's still active.
	buffered_connection::ptr other_ptr = other.lock();
	if (other_ptr)
		other_ptr->shutdown();
	
	// shut down right back.
	con.shutdown();
}
void chat_handler::socket_close(buffered_connection &con, int err)
{
	if (err)
		printf("Proxied socket closed due to error, errno: 0x%x\n", err);
	
	// shut down the other end if it's still active
	buffered_connection::ptr other_ptr = other.lock();
	if (other_ptr)
		other_ptr->shutdown();
}
