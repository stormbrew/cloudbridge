require 'socket'

# This class emulates the behaviour of TCPServer, but 'listens' on a cloudbridge
# server rather than a local interface. Otherwise attempts to have an identical
# interface to TCPServer. At least as far as mongrel uses it.
class CloudServer
	def initialize(cloud_host, cloud_port, listen_hosts = ['*'])
		@cloud_host = cloud_host
		@cloud_port = cloud_port
		@listen_hosts = {}
		listen_hosts.each {|host, key|
			@listen_hosts[host] = key
		}
		@sockopts = []
	end
	
	def setsockopt(*args)
		@sockopts.push(args)
	end
	
	def accept()
		# Connect to the cloudbridge and let it know we're available for a connection.
		# This is all entirely syncronous.
		begin
			socket = TCPSocket.new(@cloud_host, @cloud_port)
			@sockopts.each {|opt| socket.setsockopt(*opt) }
		rescue Errno::ECONNREFUSED
			sleep(0.5)
			retry
		end
		socket.write("BRIDGE / HTTP/1.1\r\n")
		@listen_hosts.each {|host, key|
			socket.write("Host: #{host}" + (key ? " #{key}" : "") + "\r\n")			
		}
		socket.write("\r\n")
		code = nil
		name = nil
		while (line = socket.gets())
			line = line.strip
			if (line == "")
				case code.to_i
				when 100 # 100 Continue, just a ping. Ignore.
					code = name = nil
					next
				when 101 # 101 Upgrade, successfuly got a connection.
					return socket
				when 504 # 504 Gateway Timeout, just retry.
					socket.close()
					return accept()
				else
					raise "HTTP BRIDGE error #{code}: #{name} waiting for connection."
				end
			end
			
			if (!code && !name)
				if (match = line.match(%r{^HTTP/1\.[01] ([0-9]{3,3}) (.*)$}))
					code = match[1]
					name = match[2]
					next
				else
					raise "Parse error in BRIDGE request reply."
				end
			end
		end
	end
	
	def close()
		# doesn't need to do anything. Yet.
	end
end