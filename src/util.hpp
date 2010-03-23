#ifndef __UTIL_GUARD__
#define __UTIL_GUARD__

#include <climits>
#include <string>
#include <list>
#include <vector>

std::vector<std::string> split(const std::string &str, const std::string &split_by, unsigned int max_count = UINT_MAX);
std::vector<unsigned char> sha1(const std::string &str);
std::string sha1_str(const std::string &str);

template <typename tIter>
inline std::string join(tIter first, tIter last, const std::string &with)
{
	std::string res;
	std::string::const_iterator with_begin = with.begin(), with_end = with.end();
	for (tIter it = first; it != last; it++)
	{
		if (it != first)
			std::copy(with_begin, with_end, std::back_inserter(res));
			
		std::copy(it->begin(), it->end(), std::back_inserter(res));
	}
	return res;
}

#endif