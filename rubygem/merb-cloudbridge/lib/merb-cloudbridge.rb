module Merb
  module Rack
    autoload :CloudBridgeMongrel, "merb/rack/adapter/cloudbridge_mongrel"
    Adapter.register %w{cmongrel cloudbridge}, :CloudBridgeMongrel
  end
end