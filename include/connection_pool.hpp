#ifndef __CONNECTION_POOL_GUARD__
#define __CONNECTION_POOL_GUARD__

#include <set>
#include <string>
#include <tr1/unordered_map>
#include <tr1/memory>
#include "evx/buffered_connection.hpp"
#include "simple_stats_driver.hpp"
#include "state_stats_driver.hpp"

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
	
	std::string stats_host;
	simple_stats_driver server_stats;
	state_stats_driver connection_stats;
	state_stats_driver host_stats;
	
	typedef std::set<std::string> known_key_set;
	known_key_set known_keys;
	
	connection find_in(const std::string &host, connection_host_map &map);
	void register_in(const std::string &host, connection con, connection_host_map &map);
	void unregister_in(const std::string &host, connection con, connection_host_map &map);
	
public:
	connection_pool(const std::string &c_stats_host);
	
	connection find_client(const std::string &host) { return find_in(host, client_pool); }
	connection find_bridge(const std::string &host) { return find_in(host, bridge_pool); }
	
	const std::string &get_stats_host() const { return stats_host; }
	simple_stats_driver &get_server_stats() { return server_stats; }
	const simple_stats_driver &get_server_stats() const { return server_stats; }
	state_stats_driver &get_connection_stats() { return connection_stats; }
	const state_stats_driver &get_connection_stats() const { return connection_stats; }
	state_stats_driver &get_host_stats() { return host_stats; }
	const state_stats_driver &get_host_stats() const { return host_stats; }
	
	const std::string &add_known_key(const std::string &host_part);
	std::string find_key(const std::string &host) const;
	
	void send_stats(evx::buffered_connection &con) const;
	
	void unregister_client(const std::string &host, connection con) { unregister_in(host, con, client_pool); }
	void register_client(const std::string &host, connection con) { register_in(host, con, client_pool); }
	void unregister_bridge(const std::string &host, connection con) { unregister_in(host, con, bridge_pool); }
	void register_bridge(const std::string &host, connection con) { register_in(host, con, bridge_pool); }
};

#endif
