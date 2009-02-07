#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include <tr1/unordered_set>

#include "evx/evx.hpp"
#include "evx/buffered_connection.hpp"

using namespace evx;

class listen_handler
{
public:
	typedef std::map<std::string, buffered_connection::weak_ptr> connection_map;
	
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

class chat_handler : public buffered_handler_base
{
private:
	buffered_connection::weak_ptr other;
	
public:
	chat_handler(buffered_connection::weak_ptr c_other)
	 : other(c_other)
	{}
	
	void data_readable(buffered_connection &con)
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
	void socket_shutdown(buffered_connection &con)
	{
		puts("Got remote shutdown, disconnecting other end.");

		// shut down the other end if it's still active.
		buffered_connection::ptr other_ptr = other.lock();
		if (other_ptr)
			other_ptr->shutdown();
		
		// shut down right back.
		con.shutdown();
	}
	void socket_close(buffered_connection &con, int err)
	{
		printf("Socket closed, errno: 0x%x\n", err);
		
		// shut down the other end if it's still active
		buffered_connection::ptr other_ptr = other.lock();
		if (other_ptr)
			other_ptr->shutdown();
	}
};

// this class is a state machine class that matches itself to another connection_finder class
// based on the first line of text passed in to socket. Two sockets with the same first line
// will be matched, and then their handlers will be replaced with the chat_handler class
// that passes messages back and forth between them.
class connection_finder : public buffered_handler_base, public std::tr1::enable_shared_from_this<connection_finder>
{
private:
	std::tr1::shared_ptr<listen_handler::connection_map> pool;
	std::tr1::shared_ptr<std::string> mapped;
	
public:	
	connection_finder(std::tr1::shared_ptr<listen_handler::connection_map> c_pool)
	 : pool(c_pool)
	{}
	
	void morph(buffered_connection &this_con, buffered_connection::ptr other_con)
	{
		buffered_connection::client_handler_ptr new_handler(new chat_handler(other_con));
		this_con.set_client_handler(new_handler);
	}
	
	void data_readable(buffered_connection &con)
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
	void socket_shutdown(buffered_connection &con)
	{
		puts("Got remote shutdown.");
		// shut down right back.
		con.shutdown();
	}
	void socket_close(buffered_connection &con, int err)
	{
		printf("Socket closed, errno: 0x%x\n", err);
	}
};

void listen_handler::operator()(struct ev_loop *loop, evx_io *watcher, int revents)
{
	struct sockaddr_in remote_addr;
	socklen_t addr_len = sizeof(remote_addr);
	int socket = accept(listen_socket, reinterpret_cast<struct sockaddr*>(&remote_addr), &addr_len);
	
	buffered_connection::create_connection(loop, socket, buffered_connection::client_handler_ptr(new connection_finder(waiting_connections)));
}

int main()
{
	struct sockaddr_in listen_addr = {0};
	int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	
	int on = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(5432);
	bind(listen_socket, reinterpret_cast<struct sockaddr*>(&listen_addr), sizeof(listen_addr));
	listen(listen_socket, 5);
	
	struct ev_loop *loop = ev_default_loop(0);
	
	evx_io listen_watcher;
	evx_init(&listen_watcher, listen_handler(listen_socket));
	ev_io_set(&listen_watcher, listen_socket, EV_READ);
	ev_io_start(loop, &listen_watcher);
	
	ev_loop(loop, 0);
}