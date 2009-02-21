$: << '../lib'

require 'rubygems'
require 'mongrel'
require 'cloud_mongrel'

 class SimpleHandler < Mongrel::HttpHandler
    def process(request, response)
      response.start(200) do |head,out|
        head["Content-Type"] = "text/plain"
        out.write("hello!\n")
      end
    end
 end

 h = Mongrel::HttpServer.new("0.0.0.0", "5432")
 h.register("/test", SimpleHandler.new)
 h.register("/files", Mongrel::DirHandler.new("."))
 h.run.join
