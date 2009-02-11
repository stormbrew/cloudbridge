#ifndef __UTIL_GUARD__
#define __UTIL_GUARD__

#include <climits>
#include <string>
#include <list>
#include <vector>

std::list<std::string> split(const std::string &str, const std::string &split_by, unsigned int max_count);
std::vector<unsigned char> sha1(const std::string &str);
std::string sha1_str(const std::string &str);

#endif