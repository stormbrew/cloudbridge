#ifndef __BUFFERED_CONNECTION_GUARD__
#define __BUFFERED_CONNECTION_GUARD__

#include <string>
#include <set>
#include <list>
#include <tr1/memory>

#include "buffer.hpp"
#include "evx.hpp"

namespace evx 
{
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
		typedef std::list<ptr> connection_list;
	
		typedef std::tr1::shared_ptr<buffered_handler_base> client_handler_ptr;
	
	private:
		struct ev_loop *loop;
		int socket;
	
		buffer read_buffer;
		buffer write_buffer;
	
		bool local_shutdown; //shutdown initiated by calling code.
		bool remote_shutdown; // shutdown initiated by remote endpoint.
	
		static connection_pool connections;
		static connection_list closed_connections;
	
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
		buffered_connection(struct ev_loop *c_loop, int c_socket, client_handler_ptr c_client_handler);
	
		void handle_shutdown();
	
		void error_close(int err);
	
		void handle(int revents);
	
	public:
		static ptr create_connection(struct ev_loop *loop, int socket, client_handler_ptr client_handler)
		{
			return *connections.insert(connections.begin(), ptr(new buffered_connection(loop, socket, client_handler)));
		}
	
		~buffered_connection();
	
		bool closed() const
		{
			return socket == -1;
		}

		void shutdown();
	
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
	
		// changes the handler for this connection. If there is data to be read on the
		// read buffer, it then notifies the handler of that fact.
		void set_client_handler(client_handler_ptr handler);
	
		template <typename tClientHandler>
		std::tr1::shared_ptr<tClientHandler> get_client_handler()
		{
			return std::tr1::dynamic_pointer_cast<tClientHandler>(client_handler);
		}
	
		// attempts to read a single line from the buffer starting at it and returns an iterator
		// to one past the end of the line (including delimiter). Puts the line (excluding delimiter)
		// in str. If there is no full line in the buffer, returns it.
		// Note that this does not advance the read buffer itself. You still need to call set_read_begin
		// with the new iterator position.
		iterator readline(iterator it, std::string &str, const std::string &linedelim = "\r\n");
	
		template <typename tIter>
		void write(tIter begin, tIter end)
		{
			if (!remote_shutdown) // the other end isn't listening anymore, don't write anything.
			{
				buffer::range write_space = write_buffer.prepare_write(end - begin);
				std::copy(begin, end, write_space.first);
				ev_io_start(loop, &write_watcher);
			}
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
}
#endif