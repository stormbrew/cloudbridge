require 'sha1'

# CloudBridge Server Implementation
module CloudBridge
	# Generate a server key for a cloudbridge instance based on the secret key and hostname specified.
	# The key grants you access to serving the host in hostname and all subdomains.
	def self.generate_key(secret_key, hostname, timestamp = Time.now())
		timestamp = timestamp.to_i
		SHA1.hexdigest("#{secret_key}:#{timestamp}:#{hostname}") + ":#{timestamp}:#{hostname}"
	end
	
	def self.secret_for_timestamp(secrets_file, for_time)
		last_secret = nil
		for_time = for_time.to_i
		File.open(secrets_file, "r") {|f|
			f.each {|line|
				timestamp, last_secret = line.strip.split(':', 2)
				# go through until we find a timestamp that is lower than the current time. Ignore any further timestamps.
				break if (timestamp.to_i < for_time)
			}
		}
		if (!last_secret)
			raise "Failed to find any secrets in the secrets file."
		end
		return last_secret
	end
	
	def self.add_secret(secrets_file, secret)
		File.open(secrets_file, "a") {|f|
			f.puts("#{Time.now.to_i}:#{secret}")
		}
	end
end