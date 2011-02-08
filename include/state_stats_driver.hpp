#ifndef __STATE_STATS_DRIVER_GUARD__
#define __STATE_STATS_DRIVER_GUARD__

#include <tr1/unordered_map>
#include <string>
#include <map>

class state_counter_holder;

// Generic class for holding and updating State counters: These tell you 
//   how many of something are in a particular state. Ie. Connections 
//   waiting for a request or handling a request.
//   Use the state_counter_holder RAII class to ensure proper counting.
class state_stats_driver
{
private:
	typedef signed long long counter_type;
	typedef std::map<char, counter_type> counter;
	typedef std::map<std::string, counter> counter_map;
	
	counter_map counters;
	
protected:
	counter_type incr(const std::string &name, char state, counter_type by = 1);

public:
	counter_type get_stat(const std::string &name, char state) const;
	counter_type get_stat_total(const std::string &name) const;
	
	typedef counter_map::const_iterator iterator;
	typedef counter::const_iterator counter_iterator;
	iterator begin() const { return counters.begin(); }
	iterator end() const { return counters.end(); }
	
	friend class state_counter_holder;
};

// RAII class for holding a reference to a stateful counter
class state_counter_holder
{
private:
	state_stats_driver &driver;
	std::string name;
	char current_state;
	
public:
	state_counter_holder(state_stats_driver &sdriver, const std::string &counter_name, char initial_state)
	 : driver(sdriver), name(counter_name), current_state(initial_state)
	{
		driver.incr(name, current_state, 1);
	}
	
	char operator=(char new_state)
	{
		driver.incr(name, current_state, -1);
		current_state = new_state;
		driver.incr(name, current_state, 1);
		return current_state;
	}
	
	~state_counter_holder()
	{
		driver.incr(name, current_state, -1);
	}
};

#endif
