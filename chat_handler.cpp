#include "chat_handler.hpp"

using namespace evx;

void chat_handler::data_readable(buffered_connection &con)
{
	buffered_connection::ptr other_ptr = other.lock();

	if (!other_ptr || other_ptr->closed())
	{
		// the other end is gone, so let's start killing this connection.
		puts("Partner connection closed, closing this connection.");
		con.shutdown();
	} else {
		// dump all of our unread data to the other end.
		puts("Got data from client, passing on.");
		buffered_connection::iterator begin = con.read_begin(), end = con.read_end();
		other_ptr->write(begin, end);
		con.set_read_begin(end);			
	}
}
void chat_handler::socket_shutdown(buffered_connection &con)
{
	puts("Got remote shutdown, disconnecting other end.");

	// shut down the other end if it's still active.
	buffered_connection::ptr other_ptr = other.lock();
	if (other_ptr)
		other_ptr->shutdown();
	
	// shut down right back.
	con.shutdown();
}
void chat_handler::socket_close(buffered_connection &con, int err)
{
	printf("Socket closed, errno: 0x%x\n", err);
	
	// shut down the other end if it's still active
	buffered_connection::ptr other_ptr = other.lock();
	if (other_ptr)
		other_ptr->shutdown();
}
