require 'bridge_mongrel'

module Merb
  module Rack
    class CloudBridgeMongrel < Mongrel
      # :api: plugin
      def self.new_server(port)
        @server = ::Mongrel::HttpServer.new(@opts[:host], port, 950, 0, 60, Merb::Config[:bridge_hosts] || [], Merb::Config[:bridge_keys] || [])
      end
    end
  end
end
