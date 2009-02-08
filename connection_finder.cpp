#include <sstream>

#include "connection_finder.hpp"
#include "chat_handler.hpp"

using namespace evx;

void connection_finder::morph(buffered_connection &this_con, buffered_connection::ptr other_con)
{
	buffered_connection::client_handler_ptr new_handler(new chat_handler(other_con));
	this_con.set_client_handler(new_handler);
}

bool connection_finder::read_headers(buffered_connection &con)
{
	std::string line;
	buffered_connection::iterator it = con.readline(con.read_begin(), line);
	if (it == con.read_begin())
		return false;
		
	std::string header_method, header_url, header_version;
	std::string::iterator start = line.begin(), space;
	
	if ((space = std::find(start, line.end(), ' ')) == line.end())
	{
		error(con, 400, "Invalid Request", "Method line was unrecognized.");
		return false;
	}
	header_method = std::string(line.begin(), space);
	std::transform(header_method.begin(), header_method.end(), header_method.begin(), toupper);
	start = space + 1;
	
	if ((space = std::find(start, line.end(), ' ')) == line.end())
	{
		error(con, 400, "Invalid Request", "Method line was unrecognized.");
		return false;
	}
	url = std::string(start, space);
	start = space + 1;
	
	header_version = std::string(start, line.end());
	if (header_version != "HTTP/1.0" && header_version != "HTTP/1.1")
	{
		error(con, 400, "Invalid Request", "Unknown HTTP Version");
		return false;
	}

	std::list<std::pair<std::string, std::string> > read_headers;
	// read in and handle all headers
	buffered_connection::iterator last_it = it;
	while (true)
	{
		it = con.readline(it, line);
		if (it == last_it) // no more input available, so bail. Retry again when there's more data.
			return false;
			
		if (line == "") // end of headers, set our data to the class and return.
		{
			std::swap(method, header_method);
			std::swap(url, header_url);
			std::swap(headers, read_headers);

			// if this is a bridge request, we want move the read position forward so when the chat_handler
			// takes over we don't spew a bridge request at the client.
			if (method == "BRIDGE")
				con.set_read_begin(it);

			return true;
		}
		
		std::pair<std::string, std::string> header;
		std::string::iterator delim;

		start = line.begin();
		delim = std::find(start, line.end(), ':');
		if (delim == line.end())
		{
			error(con, 400, "Invalid Request", "Malformed Header.");
			return false;
		}
		header.first = std::string(start, delim);
		while (*(++delim) == ' '); // skip all spaces between : and value
		header.second = std::string(delim, line.end());
		read_headers.push_back(header);
		
		last_it = it;
	}
	return false;
}

void connection_finder::parse_hosts()
{
	for (header_list::iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Host")
			hosts.push_back(it->second);
	}
}

void connection_finder::data_readable(buffered_connection &con)
{
	if (mapped)
		return; // ignore input, let the chat handler pick it up later.

	if (read_headers(con))
	{
		parse_hosts();
		
		if (hosts.size() < 1)
			return error(con, 404, "Not Found", "No Host specified. This server requires a host to be chosen.");
		
		connection_pool::connection other_con;
		if (method == "BRIDGE")
		{
			for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
			{
				other_con = pool->find_client(*it);
				if (other_con)
					break;
			}
			
			if (!other_con)
			{
				for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
				{
					pool->register_bridge(*it, con.shared_from_this());
				}
			}
		} else {
			// client end can only register to one Host.
			std::string host = hosts.front();
			
			other_con = pool->find_bridge(host);

			if (!other_con)
				pool->register_client(host, con.shared_from_this());
		}
			
		if (other_con)
		{
			std::tr1::shared_ptr<connection_finder> other_finder = other_con->get_client_handler<connection_finder>();

			morph(con, other_con);
			other_finder->morph(*other_con, con.shared_from_this());
			return;
		}
		mapped = true;
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

void connection_finder::error(evx::buffered_connection &con, int error_number, const std::string &name, const std::string &text)
{
	std::stringstream str;
	str << "HTTP/1.1 " << error_number << " " << name << "\r\n";
	str << "Connection: close\r\n";
	str << "Content-Length: " << text.length() << "\r\n";
	str << "\r\n";
	str << text;
	con.write(str.str());
	con.shutdown();
}

connection_pool::connection 
connection_pool::find_in(const std::string &host, connection_pool::connection_host_map &map)
{
	connection_host_map::iterator host_it = map.find(host);
	if (host_it != map.end())
	{
		connection_list &list = host_it->second;
		connection_list::iterator con_it = list.begin();
		while (con_it != list.end())
		{
			connection con = con_it->lock();
			if (!con || con->closed())
			{
				// remove the dead item from the list and move on.
				con_it = list.erase(con_it);
			} else {
				return con;
			}
		}
	}
	return connection();
}

void
connection_pool::register_in(const std::string &host, connection_pool::connection con, connection_pool::connection_host_map &map)
{
	connection_host_map::iterator host_it = map.find(host);
	if (host_it == map.end())
		host_it = map.insert(connection_host_map::value_type(host, connection_list())).first;
		
	host_it->second.push_back(con);
}
