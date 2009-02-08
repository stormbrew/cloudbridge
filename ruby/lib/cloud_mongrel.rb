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
	class HttpServer
		alias_method(:native_initialize, :initialize)
		
		# Modified to take a hash or array of hosts to listen on as the last argument.
		def initialize(*args)
			# If the last arg is a hash or array, take it off and use it for initializing the cloudserver.
			if (args.last.kind_of?(Hash) || args.last.kind_of?(Array))
				listen_hosts = args.pop
			end
			host, port = args[0], args[1]
			args[0], args[1] = '0.0.0.0', 0 # make mongrel listen on a random port, we're going to close it after.
			native_initialize(*args)
			@host = host
			@port = port
			@socket.close
			@socket = CloudServer.new(@host, @port)
		end
	end
end
