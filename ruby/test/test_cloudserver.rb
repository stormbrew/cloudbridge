$: << "../lib"

require "cloudserver"

server = CloudServer.new("127.0.0.1", "5432")
threads = []
5.times {|threadid|
	threads.push Thread.new {
		puts("Thread #{threadid} started")
		while (client = server.accept)
			client.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 6\r\n\r\nBlah #{threadid}")		
			client.close
		end
	}
}
threads.each {|thread| thread.join }
