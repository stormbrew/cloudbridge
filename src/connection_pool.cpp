#include <sstream>
#include <cstdio>
#include "connection_finder.hpp"
#include "connection_pool.hpp"
#include "util.hpp"

connection_pool::connection_pool(const std::string &c_stats_host)
 : stats_host(c_stats_host)
{}

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
			if (!con || con->closed() || !con->get_client_handler<connection_finder>())
			{
				// remove the dead item from the list and move on.
				list.erase(con_it);
				con_it = list.begin();
                                std::printf("Warning: Reaped a dead reference to a connection in the list for %s. Could be lingering disconnected, or could have been improperly removed.\n", host.c_str());
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
		
	host_it->second.insert(con);
}
void
connection_pool::unregister_in(const std::string &host, connection_pool::connection con, connection_pool::connection_host_map &map)
{
	connection_host_map::iterator host_it = map.find(host);
	if (host_it != map.end())
		host_it->second.erase(con);
}

const std::string &
connection_pool::add_known_key(const std::string &host_part)
{
	known_keys.insert(host_part);
}

std::string
connection_pool::find_key(const std::string &host) const
{
        // TODO: Make this more efficient than a linear scan. Especially a linear scan of all keys.
        known_key_set::iterator longest = known_keys.end();
	for (known_key_set::iterator it = known_keys.begin(); it != known_keys.end(); it++)
	{
		if (*it == host)
			return *it;

                if (longest != known_keys.end() && it->length() < longest->length())
                        continue; // skip if we've already found a longer one.
		
		std::string prefixed_host = ".";
		prefixed_host += *it;

                if (prefixed_host.length() >= it->length() && std::equal(prefixed_host.rbegin(), prefixed_host.rend(), host.rbegin()))
                        longest = it;
	}
        if (longest != known_keys.end())
            return *longest;

        return "";
}

void
connection_pool::send_stats(evx::buffered_connection &con) const
{
	std::stringstream str;
	str << "{\n";
	
	std::string prefix("  ");
	str << " \"general\": {\n";
	{
		simple_stats_driver::iterator it = server_stats.begin(), end = server_stats.end();
		while (it != end)
		{
			str << prefix << '"' << it->first << "\": " << it->second;
			if (++it != end)
				str << ",";
			str << "\n";
		}
	}
	str << " },\n \"state\": {\n";
	
	{
		state_stats_driver::iterator it = connection_stats.begin(), end = connection_stats.end();
		while (it != end)
		{
			str << prefix << '"' << it->first << "\": {\n";
			
			state_stats_driver::counter_iterator item_it = it->second.begin(), item_end = it->second.end();
			while (item_it != item_end)
			{
				str << prefix << " \"" << item_it->first << "\": " << item_it->second;
				if (++item_it != item_end)
					str << ",";
				str << "\n";
			}
			str << prefix << "}";
			if (++it != end)
				str << ",";
			str << "\n";
		}
	}
	
	str << " },\n";
	
	str << " \"host keys\": {\n";
	{
		state_stats_driver::iterator it = host_stats.begin(), end = host_stats.end();
		while (it != end)
		{
			str << prefix << '"' << it->first << "\": {\n";
			
			state_stats_driver::counter_iterator item_it = it->second.begin(), item_end = it->second.end();
			while (item_it != item_end)
			{
				str << prefix << " \"" << item_it->first << "\": " << item_it->second;
				if (++item_it != item_end)
					str << ",";
				str << "\n";
			}
			str << prefix << "}";
			if (++it != end)
				str << ",";
			str << "\n";
		}
	}
	str << " }\n";
	
	str << "}\n";
	
	con.write("HTTP/1.1 200 OK\r\n");
	con.write("Connection: close\r\n");
	con.write("Content-Type: application/json\r\n\r\n");
	con.write(str.str());

	con.shutdown();
}
