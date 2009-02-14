require 'socket'
require 'cloudbridge'

module CloudBridge
	# This class emulates the behaviour of TCPServer, but 'listens' on a cloudbridge
	# server rather than a local interface. Otherwise attempts to have an identical
	# interface to TCPServer. At least as far as mongrel uses it.
	class Server
	
		def initialize(cloud_host, cloud_port, listen_hosts = ['*'], listen_keys = [])
			@cloud_host = cloud_host
			@cloud_port = cloud_port
			@listen_hosts = listen_hosts
			@listen_keys = listen_keys
			@sockopts = []
		end
	
		def setsockopt(*args)
			@sockopts.push(args)
		end
	
		def accept()
			begin
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
				@listen_hosts.each {|host|
					socket.write("Host: #{host}\r\n")
				}
				@listen_keys.each {|key|
					socket.write("Host-Key: #{key}\r\n")
				}
				socket.write("\r\n")
				code = nil
				name = nil
				headers = []
				while (line = socket.gets())
					line = line.strip
					if (line == "")
						case code.to_i
						when 100 # 100 Continue, just a ping. Ignore.
							code = name = nil
							headers = []
							next
						when 101 # 101 Upgrade, successfuly got a connection.
							socket.write("HTTP/1.1 100 Continue\r\n\r\n") # let the server know we're still here.
							return socket
						when 503, 504 # 503 Service Unavailable or 504 Gateway Timeout, just retry.
							socket.close()
							sleep_time = headers.find {|header| header["Retry-After"] } || 5
							sleep(sleep_time)
							puts("BRIDGE server timed out or is overloaded, waiting #{sleep_time}s to try again.")
							retry
						else
							raise "HTTP BRIDGE error #{code}: #{name} waiting for connection."
						end
					end
			
					if (!code && !name) # This is the initial response line
						if (match = line.match(%r{^HTTP/1\.[01] ([0-9]{3,3}) (.*)$}))
							code = match[1]
							name = match[2]
							next
						else
							raise "Parse error in BRIDGE request reply."
						end
					else
						if (match = line.match(%r{^(.+?):\s+(.+)$}))
							headers.push({match[1] => match[2]})
						else
							raise "Parse error in BRIDGE request reply's headers."
						end
					end
				end
			end
		end
	
		def close()
			# doesn't need to do anything. Yet.
		end
	end
end