#ifndef __CONNECTION_FINDER_GUARD__
#define __CONNECTION_FINDER_GUARD__

#include <list>
#include <tr1/unordered_map>

#include "evx/buffered_connection.hpp"
#include "simple_stats_driver.hpp"
#include "state_stats_driver.hpp"

enum connection_type_t {
	type_unknown,
	type_bridge,
	type_client,
};

class connection_pool;

// this class is a state machine class that matches itself to another connection_finder class
// based on the header data passed to the connection. It then, when a match is made, replaces itself
// with a chat_handler to do the actual bridging of the connection.
class connection_finder : public evx::buffered_handler_base
{
private:
	std::tr1::shared_ptr<connection_pool> pool;
	
	connection_type_t connection_type;
	
	std::string method, url;
	
	typedef std::list<std::pair<std::string, std::string> > header_list;
	header_list headers;
	std::list<std::string> hosts;
	
	struct host_key_info
	{
		std::string hash, timestamp, host;
		explicit host_key_info(const std::string &host_key);
		bool validate(const std::string &secret, std::string client_host);
	};
	std::string host_key_secret;
	std::auto_ptr<host_key_info> host_key;
	std::auto_ptr<state_counter_holder> host_state;
	
	state_counter_holder state;
	
	bool read_headers(evx::buffered_connection &con);
	void parse_hosts();
	
	void set_timeout_by_type(evx::buffered_connection &con);
	
	void register_connection(evx::buffered_connection &this_con);
	void unregister_connection(evx::buffered_connection &this_con);
	
	void error(evx::buffered_connection &con, int error_number, const std::string &name, const std::string &text);

public:	
	connection_finder(std::tr1::shared_ptr<connection_pool> c_pool, const std::string &c_secret);
	
	// public because on failure, a backend connection will have to manipulate the
	// frontend connection depending on whether the backend is really available or not.
	void find_connection(evx::buffered_connection &con);
	void morph(evx::buffered_connection &this_con, evx::buffered_connection::ptr other_con);
	
	void registered(evx::buffered_connection &con);
	void timeout(evx::buffered_connection &con);
	void data_readable(evx::buffered_connection &con);
	void socket_shutdown(evx::buffered_connection &con);
	void socket_close(evx::buffered_connection &con, int err);
};

#endif
