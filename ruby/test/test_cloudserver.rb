$: << "../lib"

require "cloudserver"

server = CloudServer.new("127.0.0.1", "5432")
client = server.accept
while (line = client.gets)
	puts(line)
end