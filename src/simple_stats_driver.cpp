#include <numeric>
#include "simple_stats_driver.hpp"

simple_stats_driver::counter_type 
simple_stats_driver::incr(const std::string &name, simple_stats_driver::counter_type by)
{
	return counters[name] += by;
}
	
simple_stats_driver::counter_type 
simple_stats_driver::get_stat(const std::string &name) const
{
	counter_map::const_iterator it = counters.find(name);
	return it != counters.end() ? it->second : 0ll;
}