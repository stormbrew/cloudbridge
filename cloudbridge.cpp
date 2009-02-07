#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include <tr1/unordered_set>

#include "evx/evx.hpp"
#include "evx/buffered_connection.hpp"

#include "listen_handler.hpp"
#include "connection_finder.hpp"
#include "chat_handler.hpp"

using namespace evx;

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