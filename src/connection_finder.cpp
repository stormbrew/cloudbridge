#include <sstream>

#include "connection_finder.hpp"
#include "connection_pool.hpp"
#include "chat_handler.hpp"
#include "util.hpp"

using namespace evx;

connection_finder::connection_finder(std::tr1::shared_ptr<connection_pool> c_pool, const std::string &c_secret)
 : pool(c_pool), connection_type(type_unknown), host_key_secret(c_secret), 
   state(c_pool->get_connection_stats(), "Connection State", '?')
{}


void connection_finder::register_connection(buffered_connection &con)
{
	for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
	{
		if (connection_type == type_bridge)
			pool->register_bridge(*it, con.shared_from_this());
		else if (connection_type == type_client)
			pool->register_client(*it, con.shared_from_this());
	}
}
void connection_finder::unregister_connection(buffered_connection &con)
{
	for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
	{
		if (connection_type == type_bridge)
			pool->unregister_bridge(*it, con.shared_from_this());
		else if (connection_type == type_client)
			pool->unregister_client(*it, con.shared_from_this());
	}
}

void connection_finder::morph(buffered_connection &this_con, buffered_connection::ptr other_con)
{
	buffered_connection::client_handler_ptr new_handler(new chat_handler(pool, other_con, connection_type));
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

connection_finder::host_key_info::host_key_info(const std::string &host_key)
{
	std::vector<std::string> components = split(host_key, ":", 3);
	if (components.size() == 3)
	{
		hash = components[0];
		timestamp = components[1];
		host = components[2];
	}
}

bool connection_finder::host_key_info::validate(const std::string &secret, std::string client_host)
{
	if (client_host != host)
	{
		// we want to see if client_host is a subdomain of host. To do that,
		// we want to do a reverse string match on ".#{client_host}" to host.
		// if their ends match that, then we're good.
		if (client_host.length() <= host.length()) // but first, if client_host isn't longer than host,
			return false; // bail.
		
		std::string prefixed_host = ".";
		prefixed_host += host;
		if (!std::equal(prefixed_host.rbegin(), prefixed_host.rend(), client_host.rbegin()))
			return false;
	}
	
	// now check if the hash in question is actually real. (saved for last because it's the more expensive op)
	std::string compare = secret;
	compare += ":" + timestamp + ":" + host;
	std::string real_hash = sha1_str(compare);
	return real_hash == hash;
}

void connection_finder::parse_hosts()
{
	for (header_list::iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Host")
			hosts.push_back(it->second);
		else if (it->first == "Host-Key")
			host_key.reset(new host_key_info(it->second));
	}
}

void connection_finder::set_timeout_by_type(evx::buffered_connection &con)
{
	switch (connection_type)
	{
	case type_unknown:
		con.set_timeout(5); // should know what type of request this is within 0.5 seconds.
		break;
	case type_bridge:
		con.set_timeout(15); // ping the backend once every 15 seconds.
		break;
	case type_client:
		con.set_timeout(60); // give us a minute to find a backend (this is fairly long)
		break;
	}
}

void connection_finder::registered(evx::buffered_connection &con)
{
	set_timeout_by_type(con);
}
void connection_finder::timeout(evx::buffered_connection &con)
{
	switch (connection_type)
	{
	case type_unknown:
		error(con, 504, "Gateway Timeout", "Did not receive data in time.");
		con.shutdown(); // connection has been waiting in indeterminate state and is now dead.
		break;
	case type_bridge:
		con.write("HTTP/1.1 100 Continue\r\n\r\n"); // ping the other end and let the connection die if it doesn't make it.
		break;
	case type_client:
		error(con, 504, "Gateway Timeout", "Could not match to a backend server in time.");
		con.shutdown(); // backend matching timed out, so give up on it.
		break;
	}
}

void connection_finder::data_readable(buffered_connection &con)
{
	if (connection_type != type_unknown)
		return; // ignore input, let the chat handler pick it up later.
		
	con.reset_timeout();
	
	if (read_headers(con))
	{
		parse_hosts();
		
		if (hosts.size() < 1)
			return error(con, 404, "Not Found", "No Host specified. This server requires a host to be chosen.");
		
		if (method == "BRIDGE")
		{
			connection_type = type_bridge;
			state = 'b';

			if (host_key_secret.length() > 0)
			{
				if (!host_key.get())
					return error(con, 401, "Access Denied", "This server requires a host key.");
				
				for (std::list<std::string>::iterator host_it = hosts.begin(); host_it != hosts.end(); host_it++)
				{
					// validate the host key against the shared secret.
					if (!host_key->validate(host_key_secret, *host_it))
						return error(con, 401, "Access Denied", "Host key did not check out for '" + *host_it + "'. Has it expired?");
				}
			}
			// Note: we allow this to happen even if a secret key isn't set so you can gather stats without
			// generating keys. Keys with empty hashes will trigger the host to be added to the known keys
			// list and then client connections will look for it and register under them.
			if (host_key.get())
			{
				pool->add_known_key(host_key->host);
				host_state.reset(new state_counter_holder(pool->get_host_stats(), host_key->host, 'B'));
			}
			
			con.write("HTTP/1.1 100 Continue\r\n\r\n");

		} else {
			connection_type = type_client;
			state = 'c';
		}
		
		set_timeout_by_type(con);
		
		find_connection(con);
	}
}

void connection_finder::find_connection(buffered_connection &con)
{
	connection_pool::connection other_con;
	
	// bail early if there are no Host headers
	if (hosts.size() < 1)
		return;
	
	if (connection_type == type_bridge)
	{
		for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
		{
			other_con = pool->find_client(*it);
			if (other_con)
				break;
		}
	} else {
		// client end can only register to one Host (and wildcard). To prevent future zaniness, we
		// make sure that's all that's in the hosts list.
		hosts.erase(++hosts.begin(), hosts.end());
		
		// find out if this is actually a request for stats, and if so send them and return.
		if (hosts.front() == pool->get_stats_host())
		{
			state = 'I';
			return pool->send_stats(con);
		}

		// split it up by dots so we can build wildcards.
		std::vector<std::string> host_parts = split(*hosts.begin(), ".");
		std::string key = pool->find_key(*hosts.begin());
		if (key.size() > 0)
			host_state.reset(new state_counter_holder(pool->get_host_stats(), key, 'C'));
		
		// build all the potentially matching sub-wildcards
		std::vector<std::string>::iterator first = host_parts.begin()+1, last = host_parts.end();
		while (last - first > 1)
		{
			std::string part = join(first, last, ".");
			std::string wc_host = "*.";
			wc_host.append(part);
			hosts.push_back(wc_host);
			first++;
		}		
		
		hosts.push_back("*");
		
		for (std::list<std::string>::iterator it = hosts.begin(); it != hosts.end(); it++)
		{
			other_con = pool->find_bridge(*it);
			if (other_con)
				break;
		}
	}
		
	if (other_con)
	{
		std::tr1::shared_ptr<connection_finder> other_finder = other_con->get_client_handler<connection_finder>();

		buffered_connection &bridge_con = connection_type == type_bridge? con : *other_con;
		bridge_con.write("HTTP/1.1 101 Upgrade\r\n"); // taylor http versions to match.
		bridge_con.write("Upgrade: HTTP/1.1\r\n\r\n");
		
		unregister_connection(con);
		other_finder->unregister_connection(*other_con);

		// morph the backend. The frontend will be morphed by the backend connection
		// when the backend has proven itself able to take a connection (otherwise, the
		// frontend connection is left pristine so it can be taken over by another more
		// successful backend connection)
		if (connection_type == type_bridge)
			morph(con, other_con);
		else
			other_finder->morph(*other_con, con.shared_from_this());
	} else {
		register_connection(con);
	}
}

void connection_finder::socket_shutdown(buffered_connection &con)
{
	unregister_connection(con);
	// shut down right back.
	con.shutdown();
}
void connection_finder::socket_close(buffered_connection &con, int err)
{
	unregister_connection(con);
	if (err)
		printf("Waiting socket closed due to error, errno: 0x%x\n", err);
}

void connection_finder::error(evx::buffered_connection &con, int error_number, const std::string &name, const std::string &text)
{
	state = 'E';
	std::stringstream str;
	str << "HTTP/1.1 " << error_number << " " << name << "\r\n";
	str << "Connection: close\r\n";
	str << "Content-Length: " << text.length() << "\r\n";
	str << "\r\n";
	str << text;
	con.write(str.str());
	unregister_connection(con);
	con.shutdown();
}
