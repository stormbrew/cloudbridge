#ifndef __CONNECTION_FINDER_GUARD__
#define __CONNECTION_FINDER_GUARD__

#include <list>
#include <tr1/unordered_map>

#include "evx/buffered_connection.hpp"

class connection_pool;

// this class is a state machine class that matches itself to another connection_finder class
// based on the header data passed to the connection. It then, when a match is made, replaces itself
// with a chat_handler to do the actual bridging of the connection.
class connection_finder : public evx::buffered_handler_base
{
private:
	std::tr1::shared_ptr<connection_pool> pool;
	bool mapped;
	
	std::string method, url;
	
	typedef std::list<std::pair<std::string, std::string> > header_list;
	header_list headers;
	std::list<std::string> hosts;
	
	bool read_headers(evx::buffered_connection &con);
	void parse_hosts();
	
	void morph(evx::buffered_connection &this_con, evx::buffered_connection::ptr other_con);
	void error(evx::buffered_connection &con, int error_number, const std::string &name, const std::string &text);

public:	
	connection_finder(std::tr1::shared_ptr<connection_pool> c_pool)
	 : pool(c_pool), mapped(false)
	{}
	
	void data_readable(evx::buffered_connection &con);
	void socket_shutdown(evx::buffered_connection &con);
	void socket_close(evx::buffered_connection &con, int err);
};

// this class acts as a connection pool that helps identify matching connections. It has a list
// of bridge requests and client requests and does the work of registering them and putting them together.
class connection_pool
{
public:
	typedef std::tr1::shared_ptr<connection_pool> ptr;
	typedef evx::buffered_connection::ptr connection;
	
private:
	typedef evx::buffered_connection::weak_ptr weak_connection;
	typedef std::set<weak_connection> connection_list;
	typedef std::tr1::unordered_map<std::string, connection_list> connection_host_map;
	
	connection_host_map client_pool;
	connection_host_map bridge_pool;
	
	connection find_in(const std::string &host, connection_host_map &map);
	void register_in(const std::string &host, connection con, connection_host_map &map);
	
public:
	connection find_client(const std::string &host) { return find_in(host, client_pool); }
	connection find_bridge(const std::string &host) { return find_in(host, bridge_pool); }
	
	void register_client(const std::string &host, connection con) { register_in(host, con, client_pool); register_in("*", con, client_pool); }
	void register_bridge(const std::string &host, connection con) { register_in(host, con, bridge_pool); }
};

#endif