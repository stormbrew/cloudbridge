require 'rubygems'
require 'lib/cloudmachine.rb'

EventMachine::run {
	CloudMachine.start_server('localhost', 5432, ['blah.com'])
}