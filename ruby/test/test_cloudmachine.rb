$: << "../lib"

require 'rubygems'
require 'cloudmachine.rb'

EventMachine::run {
	CloudMachine.start_server('localhost', 5432, ['blah.com'])
}