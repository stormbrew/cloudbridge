# This is a version of mongrel designed to connect to a cloudbridge server
# in order to receive its connections. It is heavily based on the version of
# mongrel that swiftiply uses.

begin
	load_attempted ||= false
	require 'cloudserver'
	require 'mongrel'
rescue LoadError
	unless load_attempted
		load_attempted = true
		require 'rubygems'
		retry
	end
end

module Mongrel
	class BridgeHttpServer < HttpServer
		# Modified to take 2 arguments at the end which are a list of hosts to listen for and a list of host-keys
		# to send to the server to authenticate authority over those domains.
		def initialize(*args)
			# If the last two args are an array, take them off and use it for initializing the cloudserver.
			listen_host_keys = args.last.kind_of?(Array) &&	args.pop
			listen_hosts = args.last.kind_of?(Array) && args.pop
			
			if (!listen_hosts && listen_host_keys) # if we got only one array argument at the end, make it the hosts not the keys.
				listen_hosts, listen_host_keys = listen_host_keys, listen_hosts
			end
			# last but not least, get them from the environment or provide sensible defaults.
			listen_hosts ||= (ENV['BRIDGE_HOSTS'] && ENV['BRIDGE_HOSTS'].split(',')) || ["*"]
			listen_host_keys ||= (ENV['BRIDGE_KEYS'] && ENV['BRIDGE_KEYS'].split(',')) || []

			host, port = args[0], args[1]
			args[0], args[1] = '0.0.0.0', 0 # make mongrel listen on a random port, we're going to close it after.
			super(*args)
			@host = host
			@port = port
			@socket.close
			@socket = CloudBridge::Server.new(@host, @port, listen_hosts, listen_host_keys)
		end
	end
end
