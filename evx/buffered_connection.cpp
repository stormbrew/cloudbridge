#include <tr1/memory>
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include <string>

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ev.h>

#include "evx.hpp"
#include "buffer.hpp"

#include "buffered_connection.hpp"

namespace evx
{
	buffered_connection::connection_pool buffered_connection::connections;
	buffered_connection::connection_list buffered_connection::closed_connections;

	buffered_connection::buffered_connection(struct ev_loop *c_loop, int c_socket, client_handler_ptr c_client_handler)
		: loop(c_loop), socket(c_socket), client_handler(c_client_handler), event_handler(*this), 
		  local_shutdown(false), remote_shutdown(false)
	{
		// go into nonblocking mode
		int flags;
		if (-1 == (flags = fcntl(socket, F_GETFL, 0)))
	        flags = 0;
	    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
	
		// initialize the watcher
		evx_init(&read_watcher, event_handler);
		ev_io_set(&read_watcher, socket, EV_READ);
		ev_io_start(loop, &read_watcher);
	
		evx_init(&write_watcher, event_handler);
		ev_io_set(&write_watcher, socket, EV_WRITE);
		// don't start the write watcher until we have something to write.
	}

	void buffered_connection::handle_shutdown()
	{
		if (socket == -1)
		{
			// socket already closed, do nothing.
		
		} else if (local_shutdown && remote_shutdown) {
			// socket is completely shut down, so close it.
			client_handler->socket_close(*this, 0);
			ev_io_stop(loop, &write_watcher);
			ev_io_stop(loop, &read_watcher);
			close(socket);
			socket = -1;
			closed_connections.push_back(shared_from_this());
		
		} else if (local_shutdown) {
			// socket has been requested to shut down (our write pipe closed), so once
			// we've written everything in our buffer to it we shut it down.
			if (write_buffer.read_begin() == write_buffer.read_end())
			{
				ev_io_stop(loop, &write_watcher);
				::shutdown(socket, SHUT_WR);
			}
		
		} else if (remote_shutdown) {
			// remote and is no longer listening to its read end (our write end), so we trigger
			// write events if they're not already on and attempt to drain the write queue.
			// when it's drained, it does a local shutdown and we fully close the connection.
			ev_io_start(loop, &write_watcher);
		}
	}

	void buffered_connection::error_close(int err)
	{
		if (socket != -1)
		{
			local_shutdown = remote_shutdown = true;
			client_handler->socket_close(*this, err);
			ev_io_stop(loop, &write_watcher);
			ev_io_stop(loop, &read_watcher);
			close(socket);
			socket = -1;
			closed_connections.push_back(shared_from_this());
		}
	}

	void buffered_connection::handle(int revents)
	{
		if (revents & EV_READ)
		{
			buffer::range write_space = read_buffer.prepare_write(4096);
			int count = recv(socket, &*write_space.first, 4096, 0);
			if (count == 0)
			{
				// count of 0 means the the other end closed their write end, so we let the
				// event handler know that the other end has closed their connection to us.
				remote_shutdown = true;
				handle_shutdown();

				client_handler->socket_shutdown(*this);
				return;
			
			} else if (count == -1) {
				if (errno != EAGAIN) // there was an error on the socket, so close it down by removing it from the list.
				{
					error_close(errno);
					return; // get out of dodge, this object is probably not existent anymore.
				}
				count = 0;
			}
			read_buffer.set_write_end(write_space.first + count);
			client_handler->data_readable(*this);
		}
		if (revents & EV_WRITE)
		{
			buffer::iterator begin = write_buffer.read_begin();
			buffer::iterator end = write_buffer.read_end();
			if (begin != end)
			{
				int count = send(socket, &*begin, end - begin, 0);
				if (count == -1)
				{
					if (errno != EAGAIN) // there was an error on the socket, so close it down by removing it from the list.
					{
						error_close(errno);
						return;
					}
					count = 0;
				}
				write_buffer.set_read_begin(begin + count);
			}
			if (write_buffer.read_begin() == write_buffer.read_end())
			{
				handle_shutdown();
				ev_io_stop(loop, &write_watcher);
			}
		}
	}

	buffered_connection::~buffered_connection()
	{
		// close the connection outright
		evx_clean(&read_watcher);
		evx_clean(&write_watcher);
	}

	void buffered_connection::shutdown()
	{
		// set the shutdown flag and then encourage writing so the buffer is flushed properly.
		local_shutdown = true;
		handle_shutdown();
	}

	void buffered_connection::set_client_handler(client_handler_ptr handler)
	{
		client_handler = handler;
		if (read_begin() != read_end())
		{
			client_handler->data_readable(*this);
		}
	}

	buffered_connection::iterator buffered_connection::readline(buffered_connection::iterator it, std::string &str, const std::string &linedelim)
	{
		iterator delim = std::search(it, read_end(), linedelim.begin(), linedelim.end());
		if (delim != read_end())
		{
			str = std::string(it, delim);
			return delim + linedelim.length();
		}
		return it;
	}
}