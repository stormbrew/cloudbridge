require 'bridge_mongrel'
require 'rack/handler/mongrel'

# Replace the real httpserver class with ours.
module Mongrel
  RealHttpServer = HttpServer
  HttpServer = BridgeHttpServer
end

module Rack
  module Handler
    class CloudBridgeMongrel < Handler::Mongrel
      def self.run(app, options={})
        if (options[:BridgeHosts])
          ENV['BRIDGE_HOSTS'] = options[:BridgeHosts]
          ENV['BRIDGE_KEYS'] = options[:BridgeKeys]
        end
        super(app, options)
      end
      
      def self.options_parse(opts, options)
        super(opts, options)
        opts.on("-H", "--bridge-hosts HOST1,HOST2", "Comma-separated list of hostnames to listen on through the bridge server.") do |h|
          options[:BridgeHosts] = h
        end
        opts.on("-K", "--bridge-keys KEY1,KEY2", "Comma separated list of host authentication keys to give to the bridge server.") do |k|
          options[:BridgeKeys] = k
        end
      end
    end
  end
end
