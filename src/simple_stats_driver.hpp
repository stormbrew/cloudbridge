#ifndef __SIMPLE_STATS_DRIVER_GUARD__
#define __SIMPLE_STATS_DRIVER_GUARD__

#include <tr1/unordered_map>
#include <map>

// Generic class for holding and updating simple incremental counters: 
// These just go up. You can increment
// them. Useful for "#reqs served" or "#bytes served"
class simple_stats_driver
{
private:
	typedef signed long long counter_type;
	typedef std::map<std::string, counter_type> counter_map;
	
	counter_map counters;
	
public:
	counter_type incr(const std::string &name, counter_type by = 1);
	
	counter_type get_stat(const std::string &name) const;
	
	typedef counter_map::const_iterator iterator;
	iterator begin() const { return counters.begin(); }
	iterator end() const { return counters.end(); }
};

// RAII class for holding a reference to a simple counter
class counter_holder
{
private:
	simple_stats_driver &driver;
	std::string name;
	
public:
	counter_holder(simple_stats_driver &sdriver, const std::string &counter_name)
	 : driver(sdriver), name(counter_name)
	{
		driver.incr(name, 1ll);
	}
	
	const std::string &operator=(const std::string new_name)
	{
		driver.incr(name, -1ll);
		name = new_name;
		driver.incr(name, 1ll);
		return name;
	}
	
	~counter_holder()
	{
		driver.incr(name, -1ll);
	}
};

#endif