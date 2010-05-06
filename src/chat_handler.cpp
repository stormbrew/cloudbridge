#include "chat_handler.hpp"

using namespace evx;

void chat_handler::registered(evx::buffered_connection &con)
{
	// set a short timeout to let the backend start sending data back, we'll expand the timeout later.
	if (connection_type == type_bridge)
		con.set_timeout(0.5);
	else
		con.stop_timeout();
}
void chat_handler::timeout(evx::buffered_connection &con)
{
	// timeout means the backend failed. Close our connection and let the other end rebind elsewhere.
	buffered_connection::ptr other_ptr = other.lock();
	std::tr1::shared_ptr<connection_finder> other_finder = other_ptr->get_client_handler<connection_finder>();
	other_finder->find_connection(*other_ptr);
	other = buffered_connection::weak_ptr();
	con.error_close(0);	
}

void chat_handler::data_readable(buffered_connection &con)
{
	buffered_connection::ptr other_ptr = other.lock();
	if (!other_ptr || other_ptr->closed())
	{
		// the other end is gone, so let's start killing this connection.
		con.shutdown();
		return;
	}
	
	if (successful)
	{
		// this socket has been properly bound to another chat_handler, so
		// we just start passing data through.

		// dump all of our unread data to the other end.
		buffered_connection::iterator begin = con.read_begin(), end = con.read_end();
		other_ptr->write(begin, end);
		con.set_read_begin(end);			
	} else {
		// this socket has not been successfuly bound, so we read one line of what's
		// been read. If it's a 100-continue status code, we move the read buffer past it.
		// Then, either way, we bind the other end to this one.
		std::tr1::shared_ptr<connection_finder> other_finder = other_ptr->get_client_handler<connection_finder>();
		std::string line;
		buffered_connection::iterator it = con.readline(con.read_begin(), line), last_it = con.read_begin();
		if (it != con.read_begin())
		{
			std::string one_hundred = "HTTP/1.1 100";
			if (line.substr(0, one_hundred.length()) == one_hundred)
			{
				// get one more line and make sure it's empty.
				last_it = it;
				it = con.readline(it, line);
				if (it == last_it)
					return; // pick it up again when there's more data.
				if (line != "") // malformed, just kill the connection.
				{
					other_finder->find_connection(*other_ptr);
					other = buffered_connection::weak_ptr();
					con.error_close(0);
					return;
				}
				con.set_read_begin(it); // move the read pointer past the 100 continue.
			}
			// morph the other connection
			other_finder->morph(*other_ptr, con.shared_from_this());
			con.stop_timeout(); // no timeout once bound.
			successful = true;
		}
	}
}
void chat_handler::end_other_end(buffered_connection &con)
{
	buffered_connection::ptr other_ptr = other.lock();
	if (successful)
	{
		// shut down the other end if it's still active.
		if (other_ptr)
			other_ptr->shutdown();
		
		successful = false;
	} else if (other_ptr) {
		// let the other end find a new backend
		std::tr1::shared_ptr<connection_finder> other_finder = other_ptr->get_client_handler<connection_finder>();
		other_finder->find_connection(*other_ptr);
	}
	other = buffered_connection::weak_ptr();
}
void chat_handler::socket_shutdown(buffered_connection &con)
{
	end_other_end(con);
	
	// shut down right back.
	con.shutdown();
}
void chat_handler::socket_close(buffered_connection &con, int err)
{
	if (err)
		printf("Proxied socket closed due to error, errno: 0x%x\n", err);
	
	end_other_end(con);
}
