#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <tr1/unordered_set>

#include "evx.hpp"
#include "buffered_connection.hpp"

class data_handler : public buffered_handler_base
{
	void data_readable(buffered_connection &con)
	{
		buffered_connection::iterator begin = con.read_begin(), end = con.read_end();
		con.write(begin, end);
		con.set_read_begin(end);
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

class listen_handler
{
private:
	int listen_socket;
	struct ev_loop *loop;
	evx_io *watcher;
	
public:
	listen_handler(int c_listen_socket)
	 : listen_socket(c_listen_socket)
	{}
	
	void operator()(struct ev_loop *loop, evx_io *watcher, int revents)
	{
		struct sockaddr_in remote_addr;
		socklen_t addr_len = sizeof(remote_addr);
		int socket = accept(listen_socket, reinterpret_cast<struct sockaddr*>(&remote_addr), &addr_len);
		
		buffered_connection::create_connection(loop, socket, buffered_connection::client_handler_ptr(new data_handler));
	}
};

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