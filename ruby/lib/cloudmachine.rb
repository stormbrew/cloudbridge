# -*- coding: ISO-8859-1 -*-
require 'eventmachine'
require 'socket'
require 'strscan'

# Cloudbridge client implementation. Heavily based on Swiftiply's client, 
# but implementing the cloudbridge HTTP protocol extensions.

module CloudMachine
	# connect to a cloudbridge server and tell it we accept connections 
	# to the hosts in hostlist. Hostlist can either be an array of strings
	# or a hash of strings to strings, where the keys are hosts and the values
	# are sha1 hash keys that authorize the client to be listening on that
	# hostname.
	def self.start_server(cloudbridge, port, hostlist = ['*'], handler = nil)
		handler ||= Connection
		raise "Handler must be derived from CloudMachine::Connection." if !(handler <= Connection)
		connection = ::EventMachine.connect(cloudbridge, port, handler) do |conn|
			conn.hostname = cloudbridge
			conn.port = port
			conn.hosts = {}
			hostlist.each {|key, val|
				conn.hosts[key] = val
			}
			conn.set_comm_inactivity_timeout handler.inactivity_timeout
		end
	end

	class Connection < ::EventMachine::Connection
	
		attr_accessor :hostname, :port, :hosts
		
		def self.inactivity_timeout
			@inactivity_timeout || 60
		end
	
		def self.inactivity_timeout=(val)
			@inactivity_timeout = val
		end
	
		def connection_completed
			# use our handler to deal with the first set of data
			class <<self
				alias_method(:original_receive_data, :receive_data)
				alias_method(:receive_data, :receive_initialization_data)
			end
		
			send_data "BRIDGE / HTTP/1.1\r\n"
			hosts.each {|host, key|
				send_data "Host: #{host}" + (key ? " #{key}" : "") + "\r\n"
			}
			send_data "\r\n"
		end
	
		def receive_initialization_data(data)
			@init_data ||= StringScanner.new("")
			@init_data << data

			code = nil
			name = nil
			while (@init_data.scan(/(.*?)\r\n/))
				line = @init_data[1]
				if (line == "")
					case code.to_i
					when 100 # 100 Continue, just a ping.
						@init_data = StringScanner.new(@init_data.rest)
						code = nil
						name = nil
						next
					when 101 # 101 Upgrade; give this connection back to the derived class.
						class <<self
							alias_method(:receive_data, :original_receive_data) # put the clients handler back
						end
						original_receive_data(@init_data.rest)
						@init_data = StringScanner.new("")
						return
					when 504 # 504 Gateway Timeout; clear, disconnect, and retry later.
						@init_data = StringScanner.new("")
						close_connection
						return
					else
						raise "#{code} #{name}"
					end
				end
			
				if (!code && !name)
					match = line.match(%r{^HTTP/1\.[01] ([0-9]{3,3}) (.*)$})
					if (!match)
						raise "Invalid data from cloudbridge server"
					end
					code = match[1].to_i
					name = match[2]
				else
					# Nothing we're doing yet requires looking at the headers given by the connection.
				end
			end
			# if we get here we didn't actually find anything meaningful. Reset the data buffer and wait for more data.
			@init_data.pos = 0
		end

		def unbind
			::EventMachine.add_timer(rand(2)) {CloudMachine::start_server(@hostname,@port,@hosts,self.class)}
		end
	end
end