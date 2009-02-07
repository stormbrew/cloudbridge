#include "connection_finder.hpp"
#include "chat_handler.hpp"

using namespace evx;

void connection_finder::morph(buffered_connection &this_con, buffered_connection::ptr other_con)
{
	buffered_connection::client_handler_ptr new_handler(new chat_handler(other_con));
	this_con.set_client_handler(new_handler);
}

void connection_finder::data_readable(buffered_connection &con)
{
	if (mapped)
		return; // ignore input, let the chat handler pick it up later.
	
	std::string line;
	buffered_connection::iterator it = con.readline(con.read_begin(), line);
	if (it != con.read_begin())
	{
		// update read buffer position
		con.set_read_begin(it);
		
		printf("Incoming connection mapped to %s\n", line.c_str());
		
		// see if there's already a connection with this name attached
		listen_handler::connection_map::iterator it = pool->find(line);
		if (it != pool->end())
		{
			buffered_connection::ptr other_con = it->second.lock();
			if (other_con)
			{
				std::tr1::shared_ptr<connection_finder> other_finder = other_con->get_client_handler<connection_finder>();
			
				morph(con, other_con);
				other_finder->morph(*other_con, con.shared_from_this());
			
				pool->erase(it);
				return;
			}
		}
		(*pool)[line] = con.shared_from_this();
		mapped = std::tr1::shared_ptr<std::string>(new std::string(line));
	}
}
void connection_finder::socket_shutdown(buffered_connection &con)
{
	puts("Got remote shutdown.");
	// shut down right back.
	con.shutdown();
}
void connection_finder::socket_close(buffered_connection &con, int err)
{
	printf("Socket closed, errno: 0x%x\n", err);
}
