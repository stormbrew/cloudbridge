# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{cloudbridge}
  s.version = "0.9.5"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Graham Batty"]
  s.date = %q{2009-02-08}
  s.description = %q{CloudBridge is a self-healing minimal-configuration bridge between front-end and backend web servers. This library provides the tools needed to take advantage of it in ruby.}
  s.email = %q{info@stormbrew.ca}
  s.files = [
    "bin/cloud_mongrel_rails", 
    "lib/cloudbridge.rb", 
    "lib/bridge_mongrel.rb", 
    "lib/cloudserver.rb", 
    "test/test_cloud_mongrel.rb", 
    "test/test_cloudserver.rb"]
  s.has_rdoc = true
  s.homepage = %q{http://stormbrew.ca/projects/cloudbridge}
  s.rdoc_options = ["--inline-source", "--charset=UTF-8"]
  s.require_paths = ["lib"]
	s.bindir = "bin"
	s.executables = ["cloud_mongrel_rails", "cloud_generate_key"]
  s.rubygems_version = %q{1.3.0}
  s.summary = %q{CloudBridge is a self-healing minimal-configuration bridge between front-end and backend web servers. This library provides the tools needed to take advantage of it in ruby.}
	s.rubyforge_project = "cloudbridge"
end
