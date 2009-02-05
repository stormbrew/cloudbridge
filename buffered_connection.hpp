#include <tr1/memory>
#include <set>
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

class buffered_connection;

class buffered_handler_base
{
public:
	virtual void data_readable(buffered_connection &con) = 0;
	virtual void socket_shutdown(buffered_connection &con) = 0;
	virtual void socket_close(buffered_connection &con, int err) = 0;
};

class buffered_connection : public std::tr1::enable_shared_from_this<buffered_connection>
{
public:
	typedef std::tr1::shared_ptr<buffered_connection> ptr;
	typedef std::tr1::weak_ptr<buffered_connection> weak_ptr;
	typedef std::set<ptr> connection_pool;
	
	typedef std::tr1::shared_ptr<buffered_handler_base> client_handler_ptr;
	
private:
	struct ev_loop *loop;
	int socket;
	
	buffer read_buffer;
	buffer write_buffer;
	
	bool local_shutdown; //shutdown initiated by calling code.
	bool remote_shutdown; // shutdown initiated by remote endpoint.
	
	static connection_pool connections;
	
	client_handler_ptr client_handler;
	
	buffered_connection(const buffered_connection &); // no impl. Can't copy.
	
	struct handler
	{
		buffered_connection &owner;
		handler(buffered_connection &c_owner)
		 : owner(c_owner)
		{}
		
		void operator()(struct ev_loop *loop, evx_io *watcher, int revents)
		{
			owner.handle(revents);
		}
	};
	friend struct handler;
	
	evx_io read_watcher;
	evx_io write_watcher;
	handler event_handler;

protected:
	buffered_connection(struct ev_loop *c_loop, int c_socket, client_handler_ptr c_client_handler)
		: loop(c_loop), socket(c_socket), client_handler(c_client_handler), event_handler(*this), local_shutdown(false), remote_shutdown(false)
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
	
	void handle_shutdown()
	{
		if (local_shutdown && remote_shutdown)
		{
			// socket is completely shut down, so close it.
			client_handler->socket_close(*this, 0);
			ev_io_stop(loop, &write_watcher);
			ev_io_stop(loop, &read_watcher);
			connections.erase(shared_from_this());
			
		} else if (local_shutdown) {
			// socket has been requested to shut down (our write pipe closed), so we
			// stop writing to it.
			ev_io_stop(loop, &write_watcher);
			write_buffer.drain();
			
		} else if (remote_shutdown) {
			// remote and is no longer listening to its read end (our write end), so we trigger
			// write events if they're not already on and attempt to drain the write queue.
			// when it's drained, it does a local shutdown and we fully close the connection.
			ev_io_start(loop, &write_watcher);
		}
	}
	
	void error_close(int err)
	{
		client_handler->socket_close(*this, err);
		ev_io_stop(loop, &write_watcher);
		ev_io_stop(loop, &read_watcher);
		connections.erase(shared_from_this());
	}
	
	void handle(int revents)
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
	
public:
	~buffered_connection()
	{
		// close the connection outright
		ev_io_stop(loop, &read_watcher);
		evx_clean(&read_watcher);
		
		ev_io_stop(loop, &write_watcher);
		evx_clean(&write_watcher);
		
		close(socket);
	}

	static ptr create_connection(struct ev_loop *loop, int socket, client_handler_ptr client_handler)
	{
		return *connections.insert(connections.begin(), ptr(new buffered_connection(loop, socket, client_handler)));
	}
	
	void shutdown()
	{
		// set the shutdown flag and then encourage writing so the buffer is flushed properly.
		local_shutdown = true;
		handle_shutdown();
	}
	
	typedef buffer::iterator iterator;
	iterator read_begin()
	{
		return read_buffer.read_begin();
	}
	iterator read_end()
	{
		return read_buffer.read_end();
	}
	iterator set_read_begin(iterator new_begin)
	{
		return read_buffer.set_read_begin(new_begin);
	}
	
	template <typename tIter>
	void write(tIter begin, tIter end)
	{
		buffer::range write_space = write_buffer.prepare_write(end - begin);
		std::copy(begin, end, write_space.first);
		ev_io_start(loop, &write_watcher);
	}
	
	void write(const char *str)
	{
		size_t len = strlen(str);
		write(str, str + len);
	}
	
	void write(const std::string &str)
	{
		write(str.begin(), str.end());
	}
	void write(const std::vector<char> &str)
	{
		write(str.begin(), str.end());
	}
};