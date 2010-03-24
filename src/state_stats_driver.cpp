#include <numeric>
#include "state_stats_driver.hpp"

state_stats_driver::counter_type 
state_stats_driver::incr(const std::string &name, char state, state_stats_driver::counter_type by)
{
	return counters[name][state] += by;
}

state_stats_driver::counter_type
state_stats_driver::get_stat(const std::string &name, char state) const
{
	counter_map::const_iterator state_it = counters.find(name);
	if (state_it != counters.end())
	{
		counter::const_iterator counter_it = state_it->second.find(state);
		return counter_it != state_it->second.end() ? counter_it->second : 0ll;
	}
	return 0;
}

state_stats_driver::counter_type 
state_stats_driver::get_stat_total(const std::string &name) const
{
	counter_type count = 0ll;
	counter_map::const_iterator state_it = counters.find(name);
	if (state_it != counters.end())
	{
		counter::const_iterator it, end = state_it->second.end();
		for (it = state_it->second.begin(); it != end; it++)
			count += it->second;
	}
	return count;
}