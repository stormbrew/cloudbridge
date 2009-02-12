#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <tr1/unordered_set>

#include "evx/evx.hpp"
#include "evx/buffered_connection.hpp"

#include "listen_handler.hpp"
#include "connection_finder.hpp"
#include "chat_handler.hpp"
#include "util.hpp"

using namespace evx;

int usage(const std::string &exec_name)
{
	std::cout << exec_name << " [address1]:[port1] .. [addressN]:[portN] [-h] [-s secret_key_file]" << std::endl;
	std::cout << std::endl;
	std::cout << "\t-h\tShow this help screen." << std::endl;
	std::cout << "\t-s\tFile that contains timestamp:secret_key pairs" << std::endl;
	
	return 1;
}

typedef std::map<int, std::string> secret_map;
// Loads secret timestamp and secret key pairs from filename.
secret_map load_secret_keys(const std::string &filename)
{
	secret_map keys;
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		return keys;
	std::string line;
	while (!std::getline(file, line).eof())
	{
		std::vector<std::string> timekey = split(line, ":", 2);
		keys[atoi(timekey[0].c_str())] = timekey[1];
	}
	return keys;
}

int main(int argc, char **argv)
{
	std::list<std::string> addresses;
	std::string host_secret_key_filename, host_secret_key;
	std::string exename = argv[0];
	
	argc--;
	argv++;

	while (argc && argv[0][0] != '-')
	{
		addresses.push_back(argv[0]);
		argc--;
		argv++;
	}
	
	if (addresses.size() < 1)
	{
		addresses.push_back("*:8079");
	}

	char flag;
	while ((flag = getopt(argc+1, argv-1, "s:h")) != -1)
	{
		switch (flag)
		{
		case 's':
			host_secret_key_filename = optarg;
			break;
			
		case 'h':
		case '?':
		default:
			return usage(exename);
		}
	}
	
	if (host_secret_key_filename.length() > 0)
	{
		// TODO: Make this do something more useful.
		secret_map secrets = load_secret_keys(host_secret_key_filename);
		if (secrets.size() == 0)
		{
			printf("Secrets file had no data or did not exist\n");
			return usage(exename);
		}
		secret_map::iterator it = secrets.lower_bound(time(NULL));
		if (it == secrets.begin())
		{
			printf("Couldn't find valid secret in secrets file.\n");
			return usage(exename);
		}
		it--; // we actually want the one before what lower bound returns.
		host_secret_key = it->second;
		printf("Running in secure mode (with key from %d)\n", it->first);
	}
	
	struct ev_loop *loop = ev_default_loop(0);
	printf("Event Loop initialized: Using backend %d\n", ev_backend(loop));

	std::list<evx_io> listen_watchers;
	connection_pool::ptr connections(new connection_pool);
	
	for (std::list<std::string>::iterator it = addresses.begin(); it != addresses.end(); it++)
	{
		std::vector<std::string> addrport = split(*it, ":", 2);
		if (addrport.size() != 2)
		{
			printf("Could not parse address %s\n", it->c_str());
			return usage(exename);
		}
		
		struct addrinfo hints = {0};
		struct addrinfo *addrinfo0, *addrinfo;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		
		int err;
		if (err = getaddrinfo(addrport[0]!="*"? addrport[0].c_str() : NULL, addrport[1].c_str(), &hints, &addrinfo0))
		{
			printf("Error getting address information for %s: %s\n", it->c_str(), gai_strerror(err));
			return usage(exename);
		}
		
		for (addrinfo = addrinfo0; addrinfo; addrinfo = addrinfo->ai_next)
		{
			int listen_socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
			if (-1 == listen_socket)
			{
				printf("Error creating socket. Errno: %d\n", errno);
				return usage(exename);
			}

			int on = 1;
			setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

			if (bind(listen_socket, addrinfo->ai_addr, addrinfo->ai_addrlen))
			{
				printf("Error binding socket. Errno: %d\n", errno);
				return usage(exename);
			}
			if (listen(listen_socket, 1024))
			{
				printf("Error listening on socket. Errno: %d\n", errno);
				return usage(exename);
			}

			listen_watchers.push_back(evx_io());
			evx_io &listen_watcher = listen_watchers.back();
			
			evx_init(&listen_watcher, listen_handler(listen_socket, host_secret_key, connections));
			ev_io_set(&listen_watcher, listen_socket, EV_READ);
			ev_io_start(loop, &listen_watcher);
		}
	}	
	
	buffered_connection::register_cleanup(loop);
	
	ev_loop(loop, 0);
}