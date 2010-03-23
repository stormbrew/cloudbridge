#include "connection_finder.hpp"
#include "connection_pool.hpp"
#include "util.hpp"

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
				printf("Warning: Reaped a dead reference to a connection in the list for %s. Could be lingering disconnected, or could have been improperly removed.\n", host.c_str());
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