#include <climits>
#include <stdio.h>
#define LTC_SHA1
#define SHA1
#include <tomcrypt.h>
#include <algorithm>
#include "util.hpp"

std::list<std::string> split(const std::string &str, const std::string &split_by, unsigned int max_count = UINT_MAX)
{
	typedef std::list<std::string> string_list;
	string_list res;
	
	std::string::const_iterator it = str.begin();
	while (it != str.end())
	{
		res.push_back(std::string());
		std::string &part = res.back();
		
		std::string::const_iterator next_it = std::search(it, str.end(), split_by.begin(), split_by.end());
		std::copy(it, next_it, std::back_inserter(part));
		if (next_it != str.end()) // if we found an instance of split_by, we want to move forward to skip that as well.
			it = next_it + split_by.length();
		else // otherwise, just let the loop terminate.
			it = next_it;
	}
	return res;
}

std::vector<unsigned char> sha1(const std::string &str)
{
	std::vector<unsigned char> res(20, 0);
	hash_state hash;
	sha1_init(&hash);
	sha1_process(&hash, reinterpret_cast<const unsigned char*>(str.c_str()), str.length());
	sha1_done(&hash, &*res.begin());
	return res;
}

std::string sha1_str(const std::string &str)
{
	std::vector<unsigned char> vec = sha1(str);
	std::string res(41, 0);
	
	// this is evil evil evil, but much better than the alternatives.
	snprintf(&*res.begin(), 41, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		vec[0], vec[1], vec[2], vec[3], vec[4], vec[5], vec[6], vec[7], vec[8], vec[9],
		vec[10], vec[11], vec[12], vec[13], vec[15], vec[16], vec[17], vec[18], vec[19]);
	
	res.resize(40);
	
	return res;
}